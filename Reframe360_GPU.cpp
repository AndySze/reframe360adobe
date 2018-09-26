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
	Reframe360_GPU.cpp
	
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

#include "Reframe360.cl.h"
#include "Reframe360.h"
#include "AEGP_SuiteHandler.h"
#include "Param_Utils.h"
#include "AE_EffectSuites.h"
#include "PrGPUFilterModule.h"
#include "PrSDKVideoSegmentProperties.h"

#include "ParamUtil.h"

#if _WIN32
#include <CL/cl.h>
#else
#include <OpenCL/cl.h>
#endif
#include <cuda_runtime.h>
#include <math.h>
#include <ctime>
#include "KeyFrameManager.h"
#include "Licensing.h"

// include the aescripts licensing API (should be done *after* including the Adobe AE headers!)
#include "aescriptsLicensing.h"

#include "GumroadLicenseHandler.h"

//#define GUMROAD

#ifdef GUMROAD
namespace lic = grlic;
#else
namespace lic = aescripts;
#endif

//  CUDA KERNEL 
//  * See Reframe360.cu
#ifndef __APPLE__
extern void Reframe_CUDA ( float const *outBuf, float *destBuf, unsigned int outPitch,
	unsigned int destPitch, unsigned int width, unsigned int height,
	float* rotMat,
	float* fov,
	float* tinyplanet,
	float* rectilinear,
	int samples,
	int bilinear,
	int is16bit,
	int noLicense);
#endif

static cl_kernel sKernelCache[4];

/*
**
*/
class SDK_Reframe :
	public PrGPUFilterBase
{
private:
	int _id;
    
    void CheckError(cl_int p_Error, const char* p_Msg)
    {
        if (p_Error != CL_SUCCESS)
        {
            fprintf(stderr, "%s [%d]\n", p_Msg, p_Error);
        }
    }

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
			size_t size = strlen(kReframe360_OpenCLString);
			char const* kKernelStrings = &kReframe360_OpenCLString[0];
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

			mKernel = clCreateKernel(program, "Reframe360Kernel", &result);
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
		KeyFrameManager::getInstance().isCpuProcessing = false;
#ifdef BETA_FAIL
		time_t time_ = time(NULL);

		if (time_ > BETA_FAIL_TIME) {
			return 0;
		}
#endif
		//if (!KeyFrameManager::getInstance().isRegistered)
		//	return 0;

		int m = mNodeID;
		PPixHand properties = inFrames[0];

		csSDK_uint32 index = 0;
		mGPUDeviceSuite->GetGPUPPixDeviceIndex(properties, &index);

		// Get pixel format
		PrPixelFormat pixelFormat = PrPixelFormat_Invalid;
		mPPixSuite->GetPixelFormat(properties, &pixelFormat);
		int is16f = pixelFormat != PrPixelFormat_GPU_BGRA_4444_32f;

		int camSequenceParam = AUX_CAM_SEQUENCE;

		int samples = (int)round(GetParam(MB_SAMPLES, inRenderParams->inClipTime).mFloat64);
		samples = glm::max(1, samples);

		int cam1 = KeyFrameManager::getInstance().getPreviousCamera(NULL, mNodeID, mVideoSegmentSuite, camSequenceParam, inRenderParams->inClipTime);
		

		int cam2 = KeyFrameManager::getInstance().getNextCamera(NULL, mNodeID, mVideoSegmentSuite, camSequenceParam, inRenderParams->inClipTime);

		float shutter = GetParam(MB_SHUTTER, inRenderParams->inClipTime).mFloat64;

		float* fovs = (float*)malloc(sizeof(float)*samples);
		float* tinyplanets = (float*)malloc(sizeof(float)*samples);
		float* rectilinears = (float*)malloc(sizeof(float)*samples);
		float* rotmats = (float*)malloc(sizeof(float)*samples * 9);
#
		for (int i = 0; i < samples; i++) {
			float offset = 0;
			if (samples > 1) {
				offset = fitRange((float)i*shutter, 0, samples - 1.0f, -1.0f, 1.0f);
			}

			// read the parameters
			double main_pitch = -interpParam(MAIN_CAMERA_PITCH, inRenderParams, offset) / 180 * M_PI;
			double main_yaw = -interpParam(MAIN_CAMERA_YAW, inRenderParams, offset) / 180 * M_PI;
			double main_roll = -interpParam(MAIN_CAMERA_ROLL, inRenderParams, offset) / 180 * M_PI;
			double main_fov_mult = interpParam(MAIN_CAMERA_FOV, inRenderParams, offset);

			if (GetParam(FORCE_AUX_DISPLAY, inRenderParams->inClipTime).mBool) {
				cam1 = (int)round(GetParam(ACTIVE_AUX_CAMERA_SELECTOR, inRenderParams->inClipTime).mFloat64);
				cam2 = cam1;
			}

			double cam1_pitch = -interpParam(auxParamId(AUX_CAMERA_PITCH, cam1), inRenderParams, offset) / 180 * M_PI;
			double cam1_yaw = -interpParam(auxParamId(AUX_CAMERA_YAW, cam1), inRenderParams, offset) / 180 * M_PI;
			double cam1_roll = -interpParam(auxParamId(AUX_CAMERA_ROLL, cam1), inRenderParams, offset) / 180 * M_PI;

			double cam1_fov = interpParam(auxParamId(AUX_CAMERA_FOV, cam1), inRenderParams, offset);
			double cam1_tinyplanet = interpParam(auxParamId(AUX_CAMERA_TINYPLANET, cam1), inRenderParams, offset);
			double cam1_recti = interpParam(auxParamId(AUX_CAMERA_RECTILINEAR, cam1), inRenderParams, offset);

			double cam2_pitch = -interpParam(auxParamId(AUX_CAMERA_PITCH, cam2), inRenderParams, offset) / 180 * M_PI;
			double cam2_yaw = -interpParam(auxParamId(AUX_CAMERA_YAW, cam2), inRenderParams, offset) / 180 * M_PI;
			double cam2_roll = -interpParam(auxParamId(AUX_CAMERA_ROLL, cam2), inRenderParams, offset) / 180 * M_PI;

			double cam2_fov = interpParam(auxParamId(AUX_CAMERA_FOV, cam2), inRenderParams, offset);
			double cam2_tinyplanet = interpParam(auxParamId(AUX_CAMERA_TINYPLANET, cam2), inRenderParams, offset);
			double cam2_recti = interpParam(auxParamId(AUX_CAMERA_RECTILINEAR, cam2), inRenderParams, offset);

			double pitch = 1.0, yaw = 1.0, roll = 1.0, fov = 1.0, tinyplanet = 1.0, rectilinear = 1.0;

			float camAlpha = 0;
			if(cam1 != cam2)
				camAlpha = KeyFrameManager::getInstance().getRelativeKeyFrameAlpha(NULL, mNodeID, mVideoSegmentSuite, camSequenceParam, inRenderParams->inClipTime, inRenderParams->inRenderTicksPerFrame, offset);

			double blend = getAcceleratedCameraBlend(inRenderParams, camAlpha, offset);

			if (GetParam(FORCE_AUX_DISPLAY, inRenderParams->inClipTime).mBool) {
				blend = 0;
			}

			pitch = cam1_pitch * (1.0 - blend) + cam2_pitch * blend + main_pitch;
			yaw = cam1_yaw * (1.0 - blend) + cam2_yaw * blend + main_yaw;
			roll = cam1_roll * (1.0 - blend) + cam2_roll * blend + main_roll;
			fov = (cam1_fov * (1.0 - blend) + cam2_fov * blend) * main_fov_mult;
			tinyplanet = cam1_tinyplanet * (1.0 - blend) + cam2_tinyplanet * blend;
			rectilinear = cam1_recti * (1.0 - blend) + cam2_recti * blend;

			//mat3 main_rotMat = rotationMatrix(main_pitch, main_yaw, main_roll);
			mat3 rotMat = rotationMatrix(pitch, yaw, roll);

			memcpy(&(rotmats[i * 9]), &rotMat[0], sizeof(float) * 9);
			fovs[i] = (float)fov;
			tinyplanets[i] = (float)tinyplanet;
			rectilinears[i] = (float)rectilinear;
		}

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

		if (!outgoingFrameData)
		{
			return suiteError_Fail;
		}

		// Start CUDA or OpenCL specific code

		if (mDeviceInfo.outDeviceFramework == PrGPUDeviceFramework_CUDA) {
#ifndef __APPLE__
            float* rotMatDeviceBuf;
            mGPUDeviceSuite->AllocateDeviceMemory(index, sizeof(float) * 9 * samples, (void**)&rotMatDeviceBuf);
            cudaMemcpy((void**)rotMatDeviceBuf, (void*)(&rotmats[0]), sizeof(float) * 9 * samples, cudaMemcpyHostToDevice);
            
            float* fovDeviceBuf;
            mGPUDeviceSuite->AllocateDeviceMemory(index, sizeof(float) * samples, (void**)&fovDeviceBuf);
            cudaMemcpy((void**)fovDeviceBuf, (void*)(fovs), sizeof(float) * samples, cudaMemcpyHostToDevice);
            
            float* tinyplanetDeviceBuf;
            mGPUDeviceSuite->AllocateDeviceMemory(index, sizeof(float) * samples, (void**)&tinyplanetDeviceBuf);
            cudaMemcpy((void**)tinyplanetDeviceBuf, (void*)(tinyplanets), sizeof(float) * samples, cudaMemcpyHostToDevice);
            
            float* rectilinearDeviceBuf;
            mGPUDeviceSuite->AllocateDeviceMemory(index, sizeof(float) * samples, (void**)&rectilinearDeviceBuf);
            cudaMemcpy((void**)rectilinearDeviceBuf, (void*)(rectilinears), sizeof(float) * samples, cudaMemcpyHostToDevice);
            
            free(rotmats);
            free(fovs);
            free(tinyplanets);
            free(rectilinears);
            
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
				fovDeviceBuf, tinyplanetDeviceBuf, rectilinearDeviceBuf,
				samples, true, is16f ? 1 : 0, !KeyFrameManager::getInstance().isRegistered
			);

			mGPUDeviceSuite->FreeDeviceMemory(index, rotMatDeviceBuf);
			mGPUDeviceSuite->FreeDeviceMemory(index, fovDeviceBuf);
			mGPUDeviceSuite->FreeDeviceMemory(index, tinyplanetDeviceBuf);
			mGPUDeviceSuite->FreeDeviceMemory(index, rectilinearDeviceBuf);
	
			if ( cudaPeekAtLastError() != cudaSuccess) 			
			{
				return suiteError_Fail;
			}
#endif
		} else {
			// OpenCL device pointers
			cl_mem outgoingBuffer = (cl_mem)outgoingFrameData;
			cl_mem destBuffer = (cl_mem)destFrameData;

			int count = 0;
            
            int bilinear = 0; //TODO: make parameter
            
            cl_int error;
            
            cl_context context = (cl_context)mDeviceInfo.outContextHandle;
            cl_device_id device = (cl_device_id)mDeviceInfo.outDeviceHandle;

            error  = clSetKernelArg(mKernel, count++, sizeof(int), &width);
            error |= clSetKernelArg(mKernel, count++, sizeof(int), &height);
            
            cl_mem fov_buf = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*samples, fovs, &error);
            cl_mem tinyplanet_buf = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*samples, tinyplanets, &error);
            cl_mem rectilinear_buf = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*samples, rectilinears, &error);
            error |= clSetKernelArg(mKernel, count++, sizeof(cl_mem), &fov_buf);
            error |= clSetKernelArg(mKernel, count++, sizeof(cl_mem), &tinyplanet_buf);
            error |= clSetKernelArg(mKernel, count++, sizeof(cl_mem), &rectilinear_buf);
            
            error |= clSetKernelArg(mKernel, count++, sizeof(cl_mem), &outgoingFrameData);
            error |= clSetKernelArg(mKernel, count++, sizeof(cl_mem), &destFrameData);
            
            cl_mem rotmat_buf = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*9*samples, rotmats, &error);
            error |= clSetKernelArg(mKernel, count++, sizeof(cl_mem), &rotmat_buf);
            error |= clSetKernelArg(mKernel, count++, sizeof(int), &samples);
            error |= clSetKernelArg(mKernel, count++, sizeof(int), &bilinear);
            error |= clSetKernelArg(mKernel, count++, sizeof(int), &is16f);
            
            CheckError(error, "Unable to set kernel arguments");
            
            size_t threadBlock[2] = { 16, 16 };
            size_t grid[2] = { RoundUp(width, threadBlock[0]), RoundUp(height, threadBlock[1] )};
            
            cl_event clEvent;
            cl_int result = clEnqueueNDRangeKernel(mCommandQueue, mKernel, 2, NULL, grid, threadBlock, 0, NULL, &clEvent);
            
            clWaitForEvents(1, &clEvent);
            clReleaseMemObject(fov_buf);
            clReleaseMemObject(tinyplanet_buf);
            clReleaseMemObject(rectilinear_buf);
            clReleaseMemObject(rotmat_buf);

			if ( result != CL_SUCCESS )	
				return suiteError_Fail;
		}
		return suiteError_NoError;
	}

	CameraParams activeAuxCameraParams(PrTime time) {
		CameraParams outParams;

		int activeCam = (int)round(GetParam(ACTIVE_AUX_CAMERA_SELECTOR, time).mFloat64);

		outParams.pitch = GetParam(AUX_CAMERA_PITCH, time).mFloat64;
		outParams.yaw = GetParam(AUX_CAMERA_YAW, time).mFloat64;
		outParams.roll = GetParam(AUX_CAMERA_ROLL, time).mFloat64;
		outParams.fov = GetParam(AUX_CAMERA_FOV, time).mFloat64;
		outParams.tinyplanet = GetParam(AUX_CAMERA_TINYPLANET, time).mFloat64;
		outParams.rectilinear = GetParam(AUX_CAMERA_RECTILINEAR, time).mFloat64;

		return outParams;
	}

	CameraParams auxCameraParamsForCamera(int camera, PrTime time) {
		CameraParams outParams;

		outParams.pitch = GetParam(auxParamId(AUX_CAMERA_PITCH, camera), time).mFloat64;
		outParams.yaw = GetParam(auxParamId(AUX_CAMERA_YAW, camera), time).mFloat64;
		outParams.roll = GetParam(auxParamId(AUX_CAMERA_ROLL, camera), time).mFloat64;
		outParams.fov = GetParam(auxParamId(AUX_CAMERA_FOV, camera), time).mFloat64;
		outParams.tinyplanet = GetParam(auxParamId(AUX_CAMERA_TINYPLANET, camera), time).mFloat64;
		outParams.rectilinear = GetParam(auxParamId(AUX_CAMERA_RECTILINEAR, camera), time).mFloat64;

		return outParams;
	}

	CameraParams mainCameraParamsGPU(PrTime time) {
		CameraParams outParams;

		outParams.pitch = GetParam(MAIN_CAMERA_PITCH, time).mFloat64;
		outParams.yaw = GetParam(MAIN_CAMERA_YAW, time).mFloat64;
		outParams.roll = GetParam(MAIN_CAMERA_ROLL, time).mFloat64;

		return outParams;
	}

	double interpParam(int paramID, const PrGPUFilterRenderParams* inParams, float offset) {
		PrTime time = inParams->inClipTime;
		PrTime ticksPerFrame = inParams->inRenderTicksPerFrame;

		if (offset == 0) {
			return GetParam(paramID, time).mFloat64;
		}
		else if (offset < 0) {
			offset = -offset;
			float floor = std::floor(offset);
			float frac = offset - floor;

			return GetParam(paramID, time - (floor + 1)*ticksPerFrame).mFloat64 *frac + GetParam(paramID, time - floor * ticksPerFrame).mFloat64 *(1 - frac);
		}
		else {
			float floor = std::floor(offset);
			float frac = offset - floor;

			return GetParam(paramID, time + (floor + 1)*ticksPerFrame).mFloat64 *frac + GetParam(paramID, time + floor * ticksPerFrame).mFloat64 *(1 - frac);
		}
	}

	float getAcceleratedCameraBlend(const PrGPUFilterRenderParams* inParams, float inBlend, float offset) {

		float accel = interpParam(AUX_ACCELERATION, inParams, offset);
		float blend = inBlend;

		if (blend < 0.5) {
			blend = fitRange(blend, 0, 0.5, 0, 1);
			blend = std::pow(blend, accel);
			blend = fitRange(blend, 0, 1, 0, 0.5);
		}
		else {
			blend = fitRange(blend, 0.5, 1.0, 0, 1);
			blend = 1.0 - blend;
			blend = std::pow(blend, accel);
			blend = 1.0 - blend;
			blend = fitRange(blend, 0, 1, 0.5, 1.0);
		}

		return blend;
	}

private:

	prSuiteError GetNextKeyframeTime(
		csSDK_int32 inVideoNodeID,
		csSDK_int32 inIndex,
		PrTime inTime,
		PrTime* outKeyframeTime,
		csSDK_int32* outKeyframeInterpolationMode) {

		if (!KeyFrameManager::getInstance().isAE) {
			prSuiteError error = mVideoSegmentSuite->GetNextKeyframeTime(mNodeID, inIndex, inTime, outKeyframeTime, outKeyframeInterpolationMode);
			return error;
		}
		else {
			A_long outTime = 0;
			prSuiteError error = KeyFrameManager::getInstance().getNextKeyFrameTime(inIndex+1, inTime, &outTime);
			if (error == suiteError_NoError) {
				*outKeyframeTime = outTime;
				return error;
			}
			else
				return error;
		}
	}

	bool needsInterPolation(csSDK_int32 paramIndex, PrTime time) {
		if (isFirstKeyFrameTimeOrEarlier(paramIndex, time)) {
			return false;
		}
		else if (isLastKeyFrameTimeOrLater(paramIndex, time)) {
			return false;
		}
		else if (isExactlyOnKeyFrame(paramIndex, time)) {
			return false;
		}
		else {
			return true;
		}
	}

	bool isExactlyOnKeyFrame(csSDK_int32 paramIndex, PrTime time) {
		PrTime inTime = LONG_MIN;
#ifndef AE_OS_WIN // negative in time leads to incorrect results on mac API
        inTime = 0;
#endif
		PrTime outTime = 0;
		csSDK_int32 keyframeInterpolationMode;
		prSuiteError result = suiteError_NoError;

		while (result != suiteError_NoKeyframeAfterInTime) {
			result = GetNextKeyframeTime(mNodeID, paramIndex - 1, inTime, &outTime, &keyframeInterpolationMode);

			if (outTime == time) {
				return true;
			}
			inTime = outTime;
		}
		return false;
	}

	bool isFirstKeyFrameTimeOrEarlier(csSDK_int32 paramIndex, PrTime time) {

		PrTime inTime = LONG_MIN;
#ifndef AE_OS_WIN // negative in time leads to incorrect results on mac API
        inTime = 0;
#endif
		PrTime outTime = 0;
		csSDK_int32 keyframeInterpolationMode;
		prSuiteError result = suiteError_NoError;

		result = GetNextKeyframeTime(mNodeID, paramIndex-1, inTime, &outTime, &keyframeInterpolationMode);

		if (result == suiteError_NoKeyframeAfterInTime)
			return true;
		else if (outTime >= time)
			return true;
		else
			return false;
	}

	bool isLastKeyFrameTimeOrLater(csSDK_int32 paramIndex, PrTime time) {

		PrTime inTime = time;
		PrTime outTime = 0;
		csSDK_int32 keyframeInterpolationMode;
		prSuiteError result = suiteError_NoError;
		
		result = GetNextKeyframeTime(mNodeID, paramIndex-1, inTime, &outTime, &keyframeInterpolationMode);

		return result == suiteError_NoKeyframeAfterInTime;
	}

	PrTime getPreviousKeyFrameTime(csSDK_int32 paramIndex, PrTime time) {
		PrTime inTime = LONG_MIN;
#ifndef AE_OS_WIN // negative in time leads to incorrect results on mac API
        inTime = 0;
#endif
		PrTime outTime = 0;

		while (inTime < time) {
			outTime = inTime;
			inTime = getNextKeyFrameTime(paramIndex, inTime);
		}

		return outTime;
	}

	PrTime getNextKeyFrameTime(csSDK_int32 paramIndex, PrTime time) {
		PrTime inTime = time;
		PrTime outTime = 0;
		csSDK_int32 keyframeInterpolationMode;
		prSuiteError result = suiteError_NoError;

		result = GetNextKeyframeTime(mNodeID, paramIndex-1, inTime, &outTime, &keyframeInterpolationMode);

		if (result == suiteError_NoKeyframeAfterInTime) {
			//this should be impossible!
			throw "this should be impossible! (handled by isLast... isFirst... methods";
		}

		return outTime;
	}

	float getRelativeKeyFrameAlpha(csSDK_int32 paramIndex, PrTime currentTime, PrTime timestep, double offset) {
		if (KeyFrameManager::getInstance().isAE)
			currentTime = KeyFrameManager::getInstance().getCurrentAETime();

		PrTime offsetTime = (PrTime)(currentTime + timestep * offset);

		PrTime prevTime = getPreviousKeyFrameTime(paramIndex, currentTime);
		PrTime nextTime = getNextKeyFrameTime(paramIndex, currentTime);
		PrTime prevTimeOffset = getPreviousKeyFrameTime(paramIndex, offsetTime);
		PrTime nextTimeOffset = getNextKeyFrameTime(paramIndex, offsetTime);

		if (prevTime == prevTimeOffset && nextTime == nextTimeOffset) {
			currentTime = offsetTime;
		}

		double totalDiff = nextTime - prevTime;
		double prevDiff = currentTime - prevTime;

		double alpha = prevDiff / totalDiff;

		return (float)alpha;
	}

	int getPreviousCamera(csSDK_int32 paramIndex, PrTime time) {

		int outValue = 0;

		if (!KeyFrameManager::getInstance().isAE)
		{
			PrTime queryTime = time;

			if (needsInterPolation(paramIndex, time)) {
				queryTime = getPreviousKeyFrameTime(paramIndex, time);
			}
			outValue = (int)round(GetParam(paramIndex, queryTime).mFloat64);
		}
		else {
			PrTime queryTime = KeyFrameManager::getInstance().getCurrentAETime();
			PrTime aeTime = queryTime;

			if (needsInterPolation(paramIndex, aeTime)) {
				queryTime = getPreviousKeyFrameTime(paramIndex, aeTime);
			}
			outValue = KeyFrameManager::getInstance().getKeyFrameValue(paramIndex, queryTime);
		}

		if (outValue == 0) {
			outValue = (int)round(GetParam(paramIndex, time).mFloat64);
		}

		return  outValue;

	}

	int getNextCamera(csSDK_int32 paramIndex, PrTime time) {

		int outValue = 0;

		if (!KeyFrameManager::getInstance().isAE)
		{
			PrTime queryTime = time;

			if (needsInterPolation(paramIndex, time)) {
				queryTime = getNextKeyFrameTime(paramIndex, time);
			}
			outValue = (int)round(GetParam(paramIndex, queryTime).mFloat64);
		}
		else {
			PrTime queryTime = KeyFrameManager::getInstance().getCurrentAETime();
			PrTime aeTime = queryTime;

			if (needsInterPolation(paramIndex, aeTime)) {
				queryTime = getNextKeyFrameTime(paramIndex, aeTime);
			}
			outValue = KeyFrameManager::getInstance().getKeyFrameValue(paramIndex, queryTime);
		}

		if (outValue == 0) {
			outValue = (int)round(GetParam(paramIndex, time).mFloat64);
		}

		return  outValue;
	}


private:
	// CUDA


	// OpenCL
	cl_command_queue mCommandQueue;
	cl_kernel mKernel;
};


DECLARE_GPUFILTER_ENTRY(PrGPUFilterModule<SDK_Reframe>)
