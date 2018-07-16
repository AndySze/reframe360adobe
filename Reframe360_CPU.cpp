/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2013 Adobe Systems Incorporated                       */
/* All Rights Reserved.                                            */
/*                                                                 */
/* NOTICE:  All information contained herein is, and remains the   */
/* property of Adobe Systems Incorporated and its suppliers, if    */
/* any.  The intellectual and technical concepts contained         */
/* herein are proprietary to Adobe Systems Incorporated and its    */
/* suppliers and may be covered by U.S. and Foreign Patents,       */
/* patents in process, and are protected by trade secret or        */
/* copyright law.  Dissemination of this information or            */
/* reproduction of this material is strictly forbidden unless      */
/* prior written permission is obtained from Adobe Systems         */
/* Incorporated.                                                   */
/*                                                                 */
/*******************************************************************/

#include "Reframe360.h"
#include <map>
#include "ParamUtil.h"
#include "AEGP_SuiteHandler.h"
#include "Param_Utils.h"
#include "AE_EffectSuites.h"
#include <ctime>
#include <math.h>
#include "MathUtil.h"

using namespace std;

/*
**
*/
static PF_Err GlobalSetup(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output)
{
	out_data->my_version = PF_VERSION(PLUGIN_MAJOR_VERSION, PLUGIN_MINOR_VERSION, PLUGIN_BUG_VERSION, PLUGIN_STAGE_VERSION, PLUGIN_BUILD_VERSION);

    if (in_data->appl_id == 'PrMr')
    {
        AEFX_SuiteScoper<PF_PixelFormatSuite1> pixelFormatSuite(in_data, kPFPixelFormatSuite, kPFPixelFormatSuiteVersion1, out_data);
        (*pixelFormatSuite->ClearSupportedPixelFormats)(in_data->effect_ref);
        (*pixelFormatSuite->AddSupportedPixelFormat)(in_data->effect_ref, PrPixelFormat_BGRA_4444_32f);

        AEFX_SuiteScoper<PF_UtilitySuite4> utilitySuite(in_data, kPFUtilitySuite, kPFUtilitySuiteVersion4, out_data);
        utilitySuite->EffectWantsCheckedOutFramesToMatchRenderPixelFormat(in_data->effect_ref);
    }
	
	out_data->out_flags |= PF_OutFlag_USE_OUTPUT_EXTENT | PF_OutFlag_NON_PARAM_VARY;
	out_data->out_flags2 |= PF_OutFlag2_PRESERVES_FULLY_OPAQUE_PIXELS | PF_OutFlag2_SUPPORTS_GET_FLATTENED_SEQUENCE_DATA;

	return PF_Err_NONE;
}
/*
**
*/
static PF_Err GlobalSetdown(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output)
{
	return PF_Err_NONE;
}

/*
**
*/
static PF_Err ParamsSetup(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output)
{

	PF_ParamDef	def;

	int num_params = 1;

	AEFX_CLR_STRUCT(def);
	PF_ADD_TOPICX("Rendering Parameters", 0, MB_GRP_ID);
	num_params++;

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(
		"Motionblur Samples",
		MB_SAMPLES_MIN_VALUE,
		MB_SAMPLES_MAX_VALUE,
		MB_SAMPLES_MIN_SLIDER,
		MB_SAMPLES_MAX_SLIDER,
		MB_SAMPLES_DFLT,
		PF_Precision_INTEGER,
		PF_ValueDisplayFlag_NONE,
		0,
		MB_SAMPLES
	);
	num_params++;

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(
		"Motionblur Shutter",
		MB_SHUTTER_MIN_VALUE,
		MB_SHUTTER_MAX_VALUE,
		MB_SHUTTER_MIN_SLIDER,
		MB_SHUTTER_MAX_SLIDER,
		MB_SHUTTER_DFLT,
		PF_Precision_TENTHS,
		PF_ValueDisplayFlag_NONE,
		0,
		MB_SHUTTER
	);
	num_params++;

	AEFX_CLR_STRUCT(def);
	PF_END_TOPIC(MB_GRP_ID);
	num_params++;

	AEFX_CLR_STRUCT(def);

	PF_ADD_TOPICX("Overall Camera Parameters", 0, MAIN_CAMERA_GRP_ID);
	num_params++;


	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(
		"Overall Pitch",
		MAIN_CAMERA_PITCH_MIN_VALUE,
		MAIN_CAMERA_PITCH_MAX_VALUE,
		MAIN_CAMERA_PITCH_MIN_SLIDER,
		MAIN_CAMERA_PITCH_MAX_SLIDER,
		MAIN_CAMERA_PITCH_DFLT,
		PF_Precision_TENTHS,
		PF_ValueDisplayFlag_NONE,
		0,
		MAIN_CAMERA_PITCH
	);
	num_params++;

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(
		"Overall Yaw",
		MAIN_CAMERA_YAW_MIN_VALUE,
		MAIN_CAMERA_YAW_MAX_VALUE,
		MAIN_CAMERA_YAW_MIN_SLIDER,
		MAIN_CAMERA_YAW_MAX_SLIDER,
		MAIN_CAMERA_YAW_DFLT,
		PF_Precision_TENTHS,
		PF_ValueDisplayFlag_NONE,
		0,
		MAIN_CAMERA_YAW
	);
	num_params++;

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(
		"Overall Roll",
		MAIN_CAMERA_ROLL_MIN_VALUE,
		MAIN_CAMERA_ROLL_MAX_VALUE,
		MAIN_CAMERA_ROLL_MIN_SLIDER,
		MAIN_CAMERA_ROLL_MAX_SLIDER,
		MAIN_CAMERA_ROLL_DFLT,
		PF_Precision_TENTHS,
		PF_ValueDisplayFlag_NONE,
		0,
		MAIN_CAMERA_ROLL
	);
	num_params++;

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(
		"Overall Zoom Multiplier",
		MAIN_CAMERA_FOV_MIN_VALUE,
		MAIN_CAMERA_FOV_MAX_VALUE,
		MAIN_CAMERA_FOV_MIN_SLIDER,
		MAIN_CAMERA_FOV_MAX_SLIDER,
		MAIN_CAMERA_FOV_DFLT,
		PF_Precision_TENTHS,
		PF_ValueDisplayFlag_NONE,
		0,
		MAIN_CAMERA_FOV
	);
	num_params++;

	AEFX_CLR_STRUCT(def);
	PF_END_TOPIC(MAIN_CAMERA_GRP_ID);
	num_params++;

	AEFX_CLR_STRUCT(def);
	PF_ADD_TOPICX("Camera Animation Parameters", 0, BLEND_GRP_ID);
	num_params++;

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(
		"Camera 1",
		CAMERA_MIN_VALUE,
		CAMERA_MAX_VALUE,
		CAMERA_MIN_SLIDER,
		CAMERA_MAX_SLIDER,
		CAMERA_DFLT,
		PF_Precision_INTEGER,
		PF_ValueDisplayFlag_NONE,
		0,
		AUX_CAMERA1
	);
	num_params++;

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(
		"Camera 2",
		CAMERA_MIN_VALUE,
		CAMERA_MAX_VALUE,
		CAMERA_MIN_SLIDER,
		CAMERA_MAX_SLIDER,
		CAMERA_DFLT + 1,
		PF_Precision_INTEGER,
		PF_ValueDisplayFlag_NONE,
		0,
		AUX_CAMERA2
	);
	num_params++;

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(
		"Blend Cameras",
		0,
		1.0f,
		0,
		1.0f,
		0,
		PF_Precision_TENTHS,
		PF_ValueDisplayFlag_NONE,
		0,
		AUX_BLEND
	);
	num_params++;


	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(
		"Transition Acceleration",
		AUX_ACCELERATION_MIN_VALUE,
		AUX_ACCELERATION_MAX_VALUE,
		AUX_ACCELERATION_MIN_SLIDER,
		AUX_ACCELERATION_MAX_SLIDER,
		AUX_ACCELERATION_DFLT,
		PF_Precision_TENTHS,
		PF_ValueDisplayFlag_NONE,
		0,
		AUX_ACCELERATION
	);
	num_params++;

	AEFX_CLR_STRUCT(def);
	PF_END_TOPIC(BLEND_GRP_ID);
	num_params++;

	AEFX_CLR_STRUCT(def);
	PF_ADD_TOPICX("Camera Setup Parameters", 0, CAM_SELECTION_GRP_ID);
	num_params++;

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(
		"Select Editable Camera",
		CAMERA_MIN_VALUE,
		CAMERA_MAX_VALUE,
		CAMERA_MIN_SLIDER,
		CAMERA_MAX_SLIDER,
		CAMERA_DFLT,
		PF_Precision_INTEGER,
		PF_ValueDisplayFlag_NONE,
		PF_ParamFlag_SUPERVISE,
		ACTIVE_AUX_CAMERA_SELECTOR
	);
	num_params++;

	AEFX_CLR_STRUCT(def);

	PF_ADD_CHECKBOXX(
		"Show Editable Camera",
		false,
		0,
		FORCE_AUX_DISPLAY
	);
	num_params++;

	AEFX_CLR_STRUCT(def);
	PF_END_TOPIC(CAM_SELECTION_GRP_ID);
	num_params++;

	AEFX_CLR_STRUCT(def);
	PF_ADD_TOPICX("Editable Camera Parameters", 0, AUX_CAMERA_GRP_ID);
	num_params++;

	num_params += addAuxParams(in_data, false, 0);

	AEFX_CLR_STRUCT(def);
	PF_END_TOPIC(AUX_CAMERA_GRP_ID);
	num_params++;


	for (int i = 1; i <= CAMERA_MAX_VALUE; i++) {
		num_params += addAuxParams(in_data, true, i);
	}

	out_data->num_params = num_params;
	return PF_Err_NONE;
}

/*
**
*/
static PF_Err ParamChanged(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output,
	void* extra)
{
	PF_ProgPtr effect_ref = in_data->effect_ref;

	PF_UserChangedParamExtra* param_extra = (PF_UserChangedParamExtra*)extra;

	int selectedCam = getSelectedCamera(params);

	if (param_extra->param_index == ACTIVE_AUX_CAMERA_SELECTOR) {

		//CameraParams selectedParams = paramSet->auxCamParams[selectedCam-1];

		params[AUX_CAMERA_PITCH]->u.fs_d.value = params[AUX_CAMERA_PITCH + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_PITCH]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_YAW]->u.fs_d.value = params[AUX_CAMERA_YAW + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_YAW]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_ROLL]->u.fs_d.value = params[AUX_CAMERA_ROLL + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_ROLL]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_FOV]->u.fs_d.value = params[AUX_CAMERA_FOV + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_FOV]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_TINYPLANET]->u.fs_d.value = params[AUX_CAMERA_TINYPLANET + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_TINYPLANET]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_RECTILINEAR]->u.fs_d.value = params[AUX_CAMERA_RECTILINEAR + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_RECTILINEAR]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
	}
	else {
		float changedParam = params[param_extra->param_index]->u.fs_d.value;

		switch (param_extra->param_index) {
		case AUX_CAMERA_PITCH:
			params[AUX_CAMERA_PITCH + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value = changedParam;
			params[AUX_CAMERA_PITCH + selectedCam * AUX_PARAM_NUM + 1]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
			break;
		case AUX_CAMERA_YAW:
			params[AUX_CAMERA_YAW + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value = changedParam;
			params[AUX_CAMERA_YAW + selectedCam * AUX_PARAM_NUM + 1]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
			break;
		case AUX_CAMERA_ROLL:
			params[AUX_CAMERA_ROLL + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value = changedParam;
			params[AUX_CAMERA_ROLL + selectedCam * AUX_PARAM_NUM + 1]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
			break;
		case AUX_CAMERA_FOV:
			params[AUX_CAMERA_FOV + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value = changedParam;
			params[AUX_CAMERA_FOV + selectedCam * AUX_PARAM_NUM + 1]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
			break;
		case AUX_CAMERA_TINYPLANET:
			params[AUX_CAMERA_TINYPLANET + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value = changedParam;
			params[AUX_CAMERA_TINYPLANET + selectedCam * AUX_PARAM_NUM + 1]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
			break;
		case AUX_CAMERA_RECTILINEAR:
			params[AUX_CAMERA_RECTILINEAR + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value = changedParam;
			params[AUX_CAMERA_RECTILINEAR + selectedCam * AUX_PARAM_NUM + 1]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
			break;
		default:
			break;
		}

		//paramSet->auxCamParams[selectedCam-1] = selectedParams;
	}

	return PF_Err_NONE;
}

static double getDoubleParamValueAtOffset(int paramID, PF_InData* in_data, float offset) {
	PF_ParamDef def;

	PF_CHECKOUT_PARAM(
		in_data,
		paramID,
		(A_long)(in_data->current_time + in_data->time_step * offset),
		in_data->time_step,
		in_data->time_scale,
		&def);

	return (double)def.u.fs_d.value;
}

/*
**
*/
static PF_Err Render(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output)
{
#ifdef BETA_FAIL
	time_t time_ = time(NULL);

	if (time_ > BETA_FAIL_TIME) {
		return 0;
	}
#endif

	PF_LayerDef* outgoing = &params[0]->u.ld;
	PF_LayerDef* dest = output;

	int width = output->width;
	int height = output->height;

	int samples = (int)round(params[MB_SAMPLES]->u.fs_d.value);
	//samples = max(1, samples);
	int cam1 = (int)round(params[AUX_CAMERA1]->u.fs_d.value);
	int cam2 = (int)round(params[AUX_CAMERA2]->u.fs_d.value);
	float shutter = (int)round(params[MB_SHUTTER]->u.fs_d.value);

	float* fovs = (float*)malloc(sizeof(float)*samples);
	float* tinyplanets = (float*)malloc(sizeof(float)*samples);
	float* rectilinears = (float*)malloc(sizeof(float)*samples);
	float* rotmats = (float*)malloc(sizeof(float)*samples * 9);

	for (int i = 0; i < samples; i++) {
		float offset = 0;
		if (samples > 1) {
			offset = fitRange((float)i*shutter, 0, samples - 1.0f, -1.0f, 1.0f);
		}

		// read the parameters
		double main_pitch = -interpParam_CPU(MAIN_CAMERA_PITCH, in_data, offset) / 180 * M_PI;
		double main_yaw = -interpParam_CPU(MAIN_CAMERA_YAW, in_data, offset) / 180 * M_PI;
		double main_roll = -interpParam_CPU(MAIN_CAMERA_ROLL, in_data, offset) / 180 * M_PI;
		double main_fov_mult = interpParam_CPU(MAIN_CAMERA_FOV, in_data, offset);

		if ((bool)params[FORCE_AUX_DISPLAY]->u.bd.value){
			cam1 = (int)round(params[ACTIVE_AUX_CAMERA_SELECTOR]->u.fs_d.value);
		}

		double cam1_pitch = -interpParam_CPU(auxParamId(AUX_CAMERA_PITCH, cam1), in_data, offset) / 180 * M_PI;
		double cam1_yaw = -interpParam_CPU(auxParamId(AUX_CAMERA_YAW, cam1), in_data, offset) / 180 * M_PI;
		double cam1_roll = -interpParam_CPU(auxParamId(AUX_CAMERA_ROLL, cam1), in_data, offset) / 180 * M_PI;

		double cam1_fov = interpParam_CPU(auxParamId(AUX_CAMERA_FOV, cam1), in_data, offset);
		double cam1_tinyplanet = interpParam_CPU(auxParamId(AUX_CAMERA_TINYPLANET, cam1), in_data, offset);
		double cam1_recti = interpParam_CPU(auxParamId(AUX_CAMERA_RECTILINEAR, cam1), in_data, offset);

		double cam2_pitch = -interpParam_CPU(auxParamId(AUX_CAMERA_PITCH, cam2), in_data, offset) / 180 * M_PI;
		double cam2_yaw = -interpParam_CPU(auxParamId(AUX_CAMERA_YAW, cam2), in_data, offset) / 180 * M_PI;
		double cam2_roll = -interpParam_CPU(auxParamId(AUX_CAMERA_ROLL, cam2), in_data, offset) / 180 * M_PI;

		double cam2_fov = interpParam_CPU(auxParamId(AUX_CAMERA_FOV, cam2), in_data, offset);
		double cam2_tinyplanet = interpParam_CPU(auxParamId(AUX_CAMERA_TINYPLANET, cam2), in_data, offset);
		double cam2_recti = interpParam_CPU(auxParamId(AUX_CAMERA_RECTILINEAR, cam2), in_data, offset);

		double pitch = 1.0, yaw = 1.0, roll = 1.0, fov = 1.0, tinyplanet = 1.0, rectilinear = 1.0;

		double blend = getCameraBlend_CPU(in_data, offset);

		if ((bool)params[FORCE_AUX_DISPLAY]->u.bd.value) {
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
		
	for (int i = 0; i < samples; i++)
	{
		const char* outgoingData = (const char*)outgoing->data;

		char* destData = (char*)dest->data;

		mat3 rotMat;
		memcpy(&rotMat[0], &(rotmats[i * 9]), sizeof(float) * 9);

		float fov = fovs[i];
		float aspect = (float)width / (float)height;

		for (int y = 0; y < output->height;
			++y, destData += dest->rowbytes)
		{
			for (int x = 0; x < output->width; ++x)
			{
				vec2 uv = { (float)x / width, (float)y / height };

				vec3 dir = { 0, 0, 0 };
				dir.x = (uv.x - 0.5)*2.0;
				dir.y = (uv.y - 0.5)*2.0;
				dir.y /= aspect;
				dir.z = fov;

				vec3 tinyplanet = tinyPlanetSph(dir);
				tinyplanet = normalize(tinyplanet);

				tinyplanet = rotMat * tinyplanet;
				vec3 rectdir = rotMat * dir;

				rectdir = normalize(rectdir);

				dir = mix(fisheyeDir(dir, rotMat), tinyplanet, tinyplanets[i]);
				dir = mix(dir, rectdir, rectilinears[i]);

				vec2 iuv = polarCoord(dir);
				iuv = repairUv(iuv);

				int x_new = iuv.x * (width - 1);
				int y_new = iuv.y * (height - 1);

				iuv.x *= (width - 1);
				iuv.y *= (height - 1);

				if ((x_new < width) && (y_new < height))
				{
					const char* outgoingData_ = outgoingData + y_new * outgoing->rowbytes;

					vec4 interpCol;

					float outgoingB = outgoingData_ ? ((const float*)outgoingData_)[x_new * 4 + 0] : 0.0f;
					float outgoingG = outgoingData_ ? ((const float*)outgoingData_)[x_new * 4 + 1] : 0.0f;
					float outgoingR = outgoingData_ ? ((const float*)outgoingData_)[x_new * 4 + 2] : 0.0f;
					float outgoingA = outgoingData_ ? ((const float*)outgoingData_)[x_new * 4 + 3] : 0.0f;

					float recipNewAlpha = 1.0f;

					interpCol.x = outgoingB;
					interpCol.y = outgoingG;
					interpCol.z = outgoingR;
					interpCol.w = outgoingA;

					((float*)destData)[x * 4 + 0] += interpCol.x / samples;
					((float*)destData)[x * 4 + 1] += interpCol.y / samples;
					((float*)destData)[x * 4 + 2] += interpCol.z / samples;
					((float*)destData)[x * 4 + 3] += interpCol.w / samples;
				}

				continue;
			}
		}
	}

	free(rotmats);
	free(fovs);
	free(tinyplanets);
	free(rectilinears);

	/*const char* outgoingData = (const char*)outgoing->data;

	char* destData = (char*)dest->data;
	
	for (int y = 0; y < output->height;
		++y, outgoingData += outgoing->rowbytes, destData += dest->rowbytes)
	{
		for (int x = 0; x < output->width; ++x)
		{
			float outgoingB = outgoingData ? ((const float*)outgoingData)[x * 4 + 0] : 0.0f;
			float outgoingG = outgoingData ? ((const float*)outgoingData)[x * 4 + 1] : 0.0f;
			float outgoingR = outgoingData ? ((const float*)outgoingData)[x * 4 + 2] : 0.0f;
			float outgoingA = outgoingData ? ((const float*)outgoingData)[x * 4 + 3] : 0.0f;

			float recipNewAlpha = 1.0f;
			
			((float*)destData)[x * 4 + 0] = (outgoingB + 0.5f) * recipNewAlpha;
			((float*)destData)[x * 4 + 1] = (outgoingG) * recipNewAlpha;
			((float*)destData)[x * 4 + 2] = (outgoingR) * recipNewAlpha;
			((float*)destData)[x * 4 + 3] = recipNewAlpha;
		}
	}*/
	

	return PF_Err_NONE;
}

static PF_Err SequenceSetup(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output) {
	

	return PF_Err_NONE;
}

/*
**
*/
#ifdef MSWindows
#define DllExport   __declspec( dllexport )
#else
#define DllExport	__attribute__((visibility("default")))
#endif
extern "C" DllExport PF_Err EffectMain(
	PF_Cmd inCmd,
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* inOutput,
	void* extra)
{

	PF_Err err = PF_Err_NONE;
	switch (inCmd)
	{
	case PF_Cmd_GLOBAL_SETUP:
		err = GlobalSetup(in_data, out_data, params, inOutput);
		break;
	case PF_Cmd_GLOBAL_SETDOWN:
		err = GlobalSetdown(in_data, out_data, params, inOutput);
		break;
	case PF_Cmd_PARAMS_SETUP:
		err = ParamsSetup(in_data, out_data, params, inOutput);
		break;
	case PF_Cmd_USER_CHANGED_PARAM:
		err = ParamChanged(in_data, out_data, params, inOutput, extra);
		break;
	case PF_Cmd_SEQUENCE_SETUP:
		err = SequenceSetup(in_data, out_data, params, inOutput);
		break;
	case PF_Cmd_RENDER:
		err = Render(in_data, out_data, params, inOutput);
		break;
	}
	return err;
}