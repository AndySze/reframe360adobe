/*
** ADOBE CONFIDENTIAL
**
** Copyright 2013 Adobe
** All Rights Reserved.
**
** NOTICE: Adobe permits you to use, modify, and distribute this file in
** accordance with the terms of the Adobe license agreement accompanying
** it. If you have received this file from a source other than Adobe,
** then your use, modification, or distribution of it requires the prior
** written permission of Adobe.
*/

/*
	SDK_CrossDissolve_GPU.cpp
	
	Revision History
		
	Version		Change													Engineer	Date
	=======		======													========	======
	1.0 		Created	with OpenCL render path							shoeg		8/5/2013
	1.1			Work around a crasher in CC								zlam		9/10/2013
	1.5			Fix SDK sample to handle 16f, added in 7.2				shoeg		4/23/2014
	2.0			Integrating CUDA render path generously provided by		zlam		1/20/2015
				Rama Hoetzlein from nVidia
    2.0.1       Fixed custom build steps for CUDA on Windows            zlam        5/6/2017
*/

#include "SDK_CrossDissolve.cl.h"
#include "SDK_CrossDissolve.h"
#include "PrGPUFilterModule.h"
#include "PrSDKVideoSegmentProperties.h"

#include "CameraUtil.h"
#include "CamParamManager.h"

#if _WIN32
#include <CL/cl.h>
#else
#include <OpenCL/cl.h>
#endif
#include <cuda_runtime.h>
#include <math.h>

//  CUDA KERNEL 
//  * See SDK_CrossDissolve.cu
extern void Reframe_CUDA ( float const *outBuf, float *destBuf, unsigned int outPitch,
	unsigned int destPitch, unsigned int width, unsigned int height,
	float* rotMat,
	float fov,
	float tinyplanet,
	float rectilinear);

static cl_kernel sKernelCache[4];

/*
**
*/
class SDK_Reframe :
	public PrGPUFilterBase
{
private:
	int _id;
public:
	prSuiteError InitializeCUDA ()
	{
		// Nothing to do here. CUDA Kernel statically linked

		return suiteError_NoError;
		}

	prSuiteError InitializeOpenCL ()
		{
		if (mDeviceIndex > sizeof(sKernelCache) / sizeof(cl_kernel))  	{			
			return suiteError_Fail;		// Exceeded max device count
		}

		mCommandQueue = (cl_command_queue)mDeviceInfo.outCommandQueueHandle;

		// Load and compile the kernel - a real plugin would cache binaries to disk
		mKernel = sKernelCache[mDeviceIndex];
		if (!mKernel)
		{
			cl_int result = CL_SUCCESS;
			size_t size = strlen(kSDK_CrossDissolve_OpenCLString);
			char const* kKernelStrings = &kSDK_CrossDissolve_OpenCLString[0];
			cl_context context = (cl_context)mDeviceInfo.outContextHandle;
			cl_device_id device = (cl_device_id)mDeviceInfo.outDeviceHandle;
			cl_program program = clCreateProgramWithSource(context, 1, &kKernelStrings, &size, &result);
			if (result != CL_SUCCESS)
			{
				return suiteError_Fail;
			}

			result = clBuildProgram(program, 1, &device, "-cl-single-precision-constant -cl-fast-relaxed-math", 0, 0);
			if (result != CL_SUCCESS)
			{
				return suiteError_Fail;
			}

			mKernel = clCreateKernel(program, "CrossDissolveKernel", &result);
			if (result != CL_SUCCESS)
			{
				return suiteError_Fail;
			}

			sKernelCache[mDeviceIndex] = mKernel;
		}

		return suiteError_NoError;
	}


	virtual prSuiteError Initialize( PrGPUFilterInstance* ioInstanceData )
	{
		PrGPUFilterBase::Initialize(ioInstanceData);

		_id = mNodeID;
		CamParamManager::getInstance().setCurrentID(_id);

		CamParamManager::getInstance().initParams(_id);

		if (mDeviceInfo.outDeviceFramework == PrGPUDeviceFramework_CUDA)	
			return InitializeCUDA();			

		if (mDeviceInfo.outDeviceFramework == PrGPUDeviceFramework_OpenCL)
			return InitializeOpenCL();			

		return suiteError_Fail;			// GPUDeviceFramework unknown
	}

	prSuiteError GetFrameDependencies(
		const PrGPUFilterRenderParams* inRenderParams,
		csSDK_int32* ioQueryIndex,
		PrGPUFilterFrameDependency* outFrameRequirements)
	{
			outFrameRequirements->outDependencyType = PrGPUDependency_InputFrame;
			outFrameRequirements->outSequenceTime = inRenderParams->inSequenceTime;
			return suiteError_NoError;
	}


	prSuiteError Render(
		const PrGPUFilterRenderParams* inRenderParams,
		const PPixHand* inFrames,
		csSDK_size_t inFrameCount,
		PPixHand* outFrame)
	{
		int m = mNodeID;
		PPixHand properties = inFrames[0];

		csSDK_uint32 index = 0;
		mGPUDeviceSuite->GetGPUPPixDeviceIndex(properties, &index);

		// Get pixel format
		PrPixelFormat pixelFormat = PrPixelFormat_Invalid;
		mPPixSuite->GetPixelFormat(properties, &pixelFormat);
		int is16f = pixelFormat != PrPixelFormat_GPU_BGRA_4444_32f;

		ParamSet params = CamParamManager::getInstance().getParams(_id);
		CameraParams mainParams = params.mainCamParams;

		// read the parameters
		double main_pitch = -mainParams.pitch / 180*M_PI;
		double main_yaw = GetParam(MAIN_CAMERA_YAW, inRenderParams->inClipTime).mFloat64 / 180 * M_PI;
		double main_roll = GetParam(MAIN_CAMERA_ROLL, inRenderParams->inClipTime).mFloat64 / 180 * M_PI;

		double main_fov = GetParam(MAIN_CAMERA_FOV, inRenderParams->inClipTime).mFloat64 / 10.0f;
		double main_tinyplanet = GetParam(MAIN_CAMERA_TINYPLANET, inRenderParams->inClipTime).mFloat64;
		double main_recti = GetParam(MAIN_CAMERA_RECTILINEAR, inRenderParams->inClipTime).mFloat64;

		// Get width & height
		prRect bounds = {};
		mPPixSuite->GetBounds(properties, &bounds);
		int width = bounds.right - bounds.left;
		int height = bounds.bottom - bounds.top;

		csSDK_uint32 parNumerator = 0;
		csSDK_uint32 parDenominator = 0;
		mPPixSuite->GetPixelAspectRatio(properties, &parNumerator, &parDenominator);
		
		prFieldType fieldType = 0;
		mPPix2Suite->GetFieldOrder(properties, &fieldType);

		// Create a frame to process output
		mGPUDeviceSuite->CreateGPUPPix(
			index,
			pixelFormat,
			width,
			height,
			parNumerator,
			parDenominator,
			fieldType,
			outFrame);

		if (!outFrame)
		{
			return suiteError_Fail;
		}

		// Get outgoing data
		void* outgoingFrameData = 0;
		csSDK_int32 outgoingRowBytes = 0;
		if (inFrames[0])
		{
			mGPUDeviceSuite->GetGPUPPixData(inFrames[0], &outgoingFrameData);
			mPPixSuite->GetRowBytes(inFrames[0], &outgoingRowBytes);
		}
		int outgoingPitch = outgoingRowBytes / GetGPUBytesPerPixel(pixelFormat);


		// Get dest data
		void* destFrameData = 0;
		csSDK_int32 destRowBytes = 0;
		mGPUDeviceSuite->GetGPUPPixData(*outFrame, &destFrameData);
		mPPixSuite->GetRowBytes(*outFrame, &destRowBytes);
		int destPitch = destRowBytes / GetGPUBytesPerPixel(pixelFormat);

		mat3 rotMat = rotationMatrix(main_pitch, main_yaw, main_roll);

		float* rotMatDeviceBuf;
		mGPUDeviceSuite->AllocateDeviceMemory(index, sizeof(float) * 9, (void**)&rotMatDeviceBuf);
		cudaMemcpy((void**)rotMatDeviceBuf, (void*)(&rotMat[0]), sizeof(float) * 9, cudaMemcpyHostToDevice);

		if (!outgoingFrameData)
		{
			return suiteError_Fail;
		}

		// Start CUDA or OpenCL specific code

		if (mDeviceInfo.outDeviceFramework == PrGPUDeviceFramework_CUDA) {
			
			// CUDA device pointers
			float* outgoingBuffer = (float*) outgoingFrameData;		
			float* destBuffer = (float*) destFrameData;	

			// Launch CUDA kernel
			Reframe_CUDA ( outgoingBuffer, 
								destBuffer, 
								outgoingPitch,
								destPitch,
								width, 
								height,
				rotMatDeviceBuf,
				main_fov, main_tinyplanet, main_recti
			);
	
			if ( cudaPeekAtLastError() != cudaSuccess) 			
			{
				return suiteError_Fail;
			}

		} else {
			// OpenCL device pointers
			cl_mem outgoingBuffer = (cl_mem)outgoingFrameData;
			cl_mem destBuffer = (cl_mem)destFrameData;

			float progress = 0;

			// Set the arguments
			clSetKernelArg(mKernel, 0, sizeof(cl_mem), &outgoingBuffer);
			clSetKernelArg(mKernel, 2, sizeof(cl_mem), &destBuffer);
			clSetKernelArg(mKernel, 3, sizeof(unsigned int), &outgoingPitch);
			clSetKernelArg(mKernel, 5, sizeof(unsigned int), &destPitch);
			clSetKernelArg(mKernel, 6, sizeof(int), &is16f);
			clSetKernelArg(mKernel, 7, sizeof(unsigned int), &width);
			clSetKernelArg(mKernel, 8, sizeof(unsigned int), &height);
			clSetKernelArg(mKernel, 9, sizeof(float), &progress);

			// Launch the kernel
			size_t threadBlock[2] = { 16, 16 };
			size_t grid[2] = { RoundUp(width, threadBlock[0]), RoundUp(height, threadBlock[1] )};

			cl_int result = clEnqueueNDRangeKernel(
				mCommandQueue,
				mKernel,
				2,
				0,
				grid,
				threadBlock,
				0,
				0,
				0);

			if ( result != CL_SUCCESS )	
				return suiteError_Fail;
		}
		return suiteError_NoError;
	}

private:
	// CUDA


	// OpenCL
	cl_command_queue mCommandQueue;
	cl_kernel mKernel;
};


DECLARE_GPUFILTER_ENTRY(PrGPUFilterModule<SDK_Reframe>)
