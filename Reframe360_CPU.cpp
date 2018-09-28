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
#include "KeyFrameManager.h"
#include "GumroadLicenseHandler.h"

// ********************* aescripts licensing specific code start *********************

// change the following settings for your own plugin!!!

#define LIC_PRODUCT_NAME "Reframe360"

#define LIC_PRIVATE_NUM 635489

#define LIC_PRODUCT_ID "VSRF"

#define LIC_FILENAME "Reframe360"

// set this define if your plugin is compiled for beta-testers, it will then accept BTA type licenses
#define LIC_BETA

// include the aescripts licensing API (should be done *after* including the Adobe AE headers!)
#include "aescriptsLicensing.h"
// include the aescripts licensing Adobe helper API (should be done *after* including the Adobe AE headers and the licensing API!)
#include "aescriptsLicensing_AdobeHelpers.h"

//#define GUMROAD

#ifdef GUMROAD
#include "GumroadLicenseHandler.h"

#include "GumroadLicense_AdobeHelpers.h"

namespace lic = grlic;
#else
namespace lic = aescripts;
#endif


// ********************* aescripts licensing specific code end *********************

using namespace std;

static void storeParamKeyframes(PF_InData* in_data, PF_ParamDef* params[], PF_OutData* out_data);

static void fillParamStructs(int samples, float shutter, PF_InData * in_data, PF_ParamDef ** params, int &cam1, int &cam2, float * rotmats, float * fovs, float * tinyplanets, float * rectilinears);

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
	
	out_data->out_flags |= PF_OutFlag_USE_OUTPUT_EXTENT | PF_OutFlag_NON_PARAM_VARY | PF_OutFlag_I_DO_DIALOG;
	out_data->out_flags2 |= PF_OutFlag2_PRESERVES_FULLY_OPAQUE_PIXELS | PF_OutFlag2_SUPPORTS_SMART_RENDER | PF_OutFlag2_FLOAT_COLOR_AWARE;

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
		"Camera Sequence",
		CAMERA_MIN_VALUE,
		CAMERA_MAX_VALUE,
		CAMERA_MIN_SLIDER,
		CAMERA_MAX_SLIDER,
		CAMERA_DFLT,
		PF_Precision_INTEGER,
		PF_ValueDisplayFlag_NONE,
		PF_ParamFlag_SUPERVISE,
		AUX_CAM_SEQUENCE
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

	AEFX_CLR_STRUCT(def);
	PF_ADD_BUTTON("Copy Cam Parameters", "Copy", 0, PF_ParamFlag_SUPERVISE, AUX_CAMERA_COPY_BTN_ID);
	num_params++;

	AEFX_CLR_STRUCT(def);
	PF_ADD_BUTTON("Paste Cam Parameters", "Paste", 0, PF_ParamFlag_SUPERVISE, AUX_CAMERA_PASTE_BTN_ID);
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
	AEFX_SuiteScoper<PF_HandleSuite1> handleSuite(in_data, kPFHandleSuite, kPFHandleSuiteVersion1, out_data);
	AEFX_SuiteScoper<PF_ParamUtilsSuite3> paramUtilsSuite(in_data, kPFParamUtilsSuite, kPFParamUtilsSuiteVersion3, out_data);



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
	else if (param_extra->param_index == AUX_CAMERA_COPY_BTN_ID) {
		KeyFrameManager::getInstance().setCamToCopy(selectedCam);
	}
	else if (param_extra->param_index == AUX_CAMERA_PASTE_BTN_ID) {
		int copiedCam = KeyFrameManager::getInstance().getCamToCopy();

		params[AUX_CAMERA_PITCH + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value = params[AUX_CAMERA_PITCH + copiedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_PITCH + selectedCam * AUX_PARAM_NUM + 1]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
		params[AUX_CAMERA_PITCH]->u.fs_d.value = params[AUX_CAMERA_PITCH + copiedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_PITCH]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_YAW + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value = params[AUX_CAMERA_YAW + copiedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_YAW + selectedCam * AUX_PARAM_NUM + 1]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
		params[AUX_CAMERA_YAW]->u.fs_d.value = params[AUX_CAMERA_YAW + copiedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_YAW]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_ROLL + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value = params[AUX_CAMERA_ROLL + copiedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_ROLL + selectedCam * AUX_PARAM_NUM + 1]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
		params[AUX_CAMERA_ROLL]->u.fs_d.value = params[AUX_CAMERA_ROLL + copiedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_ROLL]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_FOV + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value = params[AUX_CAMERA_FOV + copiedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_FOV + selectedCam * AUX_PARAM_NUM + 1]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
		params[AUX_CAMERA_FOV]->u.fs_d.value = params[AUX_CAMERA_FOV + copiedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_FOV]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_TINYPLANET + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value = params[AUX_CAMERA_TINYPLANET + copiedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_TINYPLANET + selectedCam * AUX_PARAM_NUM + 1]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
		params[AUX_CAMERA_TINYPLANET]->u.fs_d.value = params[AUX_CAMERA_TINYPLANET + copiedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_TINYPLANET]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_RECTILINEAR + selectedCam * AUX_PARAM_NUM + 1]->u.fs_d.value = params[AUX_CAMERA_RECTILINEAR + copiedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
		params[AUX_CAMERA_RECTILINEAR + selectedCam * AUX_PARAM_NUM + 1]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
		params[AUX_CAMERA_RECTILINEAR]->u.fs_d.value = params[AUX_CAMERA_RECTILINEAR + copiedCam * AUX_PARAM_NUM + 1]->u.fs_d.value;
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
	}

	return PF_Err_NONE;
}

static void storeParamKeyframes(PF_InData* in_data, PF_ParamDef* params[], PF_OutData* out_data) {
	AEFX_SuiteScoper<PF_ParamUtilsSuite3> paramUtilsSuite(in_data, kPFParamUtilsSuite, kPFParamUtilsSuiteVersion3, out_data);
	PF_ProgPtr effect_ref = in_data->effect_ref;

	KeyFrameManager::getInstance().setCurrentAETime(in_data->current_time);

	if (in_data->appl_id != 'PrMr') {
		KeyFrameManager::getInstance().isAE = true;
	}
	else {
		return;
	}

	PF_KeyIndex keyCount;

	paramUtilsSuite->PF_GetKeyframeCount(effect_ref, AUX_CAM_SEQUENCE, &keyCount);

	KeyFrameManager::getInstance().beginKeyNewKeyframeData();

	for (int i = 0; i < keyCount; i++) {
		A_long keyTime;
		A_u_long timeScale;
		PF_ParamDef paramDef;

		paramUtilsSuite->PF_CheckoutKeyframe(effect_ref, AUX_CAM_SEQUENCE, i, &keyTime, &timeScale, &paramDef);

		float value = paramDef.u.fs_d.value;

		KeyFrameManager::getInstance().addKeyFrame(AUX_CAM_SEQUENCE, keyTime, (int)value);

		int a = 0;
	}
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



void fillParamStructs(int samples, float shutter, PF_InData * in_data, PF_ParamDef ** params, int &cam1, int &cam2, float * rotmats, float * fovs, float * tinyplanets, float * rectilinears)
{
	PrTime prevDiff, nextDiff;
	if (KeyFrameManager::getInstance().isCpuProcessing && !KeyFrameManager::getInstance().isAE) {
		cam1 = KeyFrameManager::getInstance().getPreviousCamera_Pr_CPU(in_data, prevDiff);
		cam2 = KeyFrameManager::getInstance().getNextCamera_Pr_CPU(in_data, nextDiff);
	}
	else {
		cam1 = KeyFrameManager::getInstance().getPreviousCamera(params, NULL, NULL, AUX_CAM_SEQUENCE, in_data->current_time);
		cam2 = KeyFrameManager::getInstance().getNextCamera(params, NULL, NULL, AUX_CAM_SEQUENCE, in_data->current_time);
	}

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

		if ((bool)params[FORCE_AUX_DISPLAY]->u.bd.value) {
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

		float camAlpha = 0;
		if (cam1 != cam2) {
			if (KeyFrameManager::getInstance().isAE || ! KeyFrameManager::getInstance().isCpuProcessing)
				camAlpha = KeyFrameManager::getInstance().getRelativeKeyFrameAlpha(params, NULL, NULL, AUX_CAM_SEQUENCE, in_data->current_time, in_data->time_step, offset);
			else
				camAlpha = (float)prevDiff / (prevDiff + nextDiff);
		}

		double blend = getCameraBlend_CPU(in_data, camAlpha, offset);

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
		rotMat = transpose(rotMat);

		memcpy(&(rotmats[i * 9]), &rotMat[0], sizeof(float) * 9);
		fovs[i] = (float)fov;
		tinyplanets[i] = (float)tinyplanet;
		rectilinears[i] = (float)rectilinear;
	}
}

static inline vec4 read32bitVec4(const char* outgoingRowData, int x_new) {
	vec4 outgoing;

	outgoing.x = outgoingRowData ? ((const float*)outgoingRowData)[x_new * 4 + 0] : 0.0f;
	outgoing.y = outgoingRowData ? ((const float*)outgoingRowData)[x_new * 4 + 1] : 0.0f;
	outgoing.z = outgoingRowData ? ((const float*)outgoingRowData)[x_new * 4 + 2] : 0.0f;
	outgoing.w = outgoingRowData ? ((const float*)outgoingRowData)[x_new * 4 + 3] : 0.0f;

	return outgoing;
}

static inline void write32bitVec4(char* destData, int x, int y, int rowbytes, vec4 value) {
	((float*)destData)[y*rowbytes / sizeof(float) + x * 4 + 0] = value.x;
	((float*)destData)[y*rowbytes / sizeof(float) + x * 4 + 1] = value.y;
	((float*)destData)[y*rowbytes / sizeof(float) + x * 4 + 2] = value.z;
	((float*)destData)[y*rowbytes / sizeof(float) + x * 4 + 3] = value.w;
}

static inline vec4 read16bitVec4(const char* outgoingRowData, int x_new) {
	vec4 outgoing;

	outgoing.x = outgoingRowData ? ((const A_u_short*)outgoingRowData)[x_new * 4 + 0] / (float)USHRT_MAX : 0.0f;
	outgoing.y = outgoingRowData ? ((const A_u_short*)outgoingRowData)[x_new * 4 + 1] / (float)USHRT_MAX : 0.0f;
	outgoing.z = outgoingRowData ? ((const A_u_short*)outgoingRowData)[x_new * 4 + 2] / (float)USHRT_MAX : 0.0f;
	outgoing.w = outgoingRowData ? ((const A_u_short*)outgoingRowData)[x_new * 4 + 3] / (float)USHRT_MAX : 0.0f;

	return outgoing;
}

static inline void write16bitVec4(char* destData, int x, int y, int rowbytes, vec4 value) {
	((A_u_short*)destData)[y*rowbytes / sizeof(A_u_short) + x * 4 + 0] = MIN(value.x * USHRT_MAX, USHRT_MAX);
	((A_u_short*)destData)[y*rowbytes / sizeof(A_u_short) + x * 4 + 1] = MIN(value.y * USHRT_MAX, USHRT_MAX);
	((A_u_short*)destData)[y*rowbytes / sizeof(A_u_short) + x * 4 + 2] = MIN(value.z * USHRT_MAX, USHRT_MAX);
	((A_u_short*)destData)[y*rowbytes / sizeof(A_u_short) + x * 4 + 3] = MIN(value.w * USHRT_MAX, USHRT_MAX);
}

static inline vec4 read8bitVec4(const char* outgoingRowData, int x_new) {
	vec4 outgoing;

	outgoing.x = outgoingRowData ? ((const A_u_char*)outgoingRowData)[x_new * 4 + 0] / (float)UCHAR_MAX : 0.0f;
	outgoing.y = outgoingRowData ? ((const A_u_char*)outgoingRowData)[x_new * 4 + 1] / (float)UCHAR_MAX : 0.0f;
	outgoing.z = outgoingRowData ? ((const A_u_char*)outgoingRowData)[x_new * 4 + 2] / (float)UCHAR_MAX : 0.0f;
	outgoing.w = outgoingRowData ? ((const A_u_char*)outgoingRowData)[x_new * 4 + 3] / (float)UCHAR_MAX : 0.0f;

	return outgoing;
}

static inline void write8bitVec4(char* destData, int x, int y, int rowbytes, vec4 value) {
	((A_u_char*)destData)[y*rowbytes / sizeof(A_u_char) + x * 4 + 0] = MIN(value.x * UCHAR_MAX, UCHAR_MAX);
	((A_u_char*)destData)[y*rowbytes / sizeof(A_u_char) + x * 4 + 1] = MIN(value.y * UCHAR_MAX, UCHAR_MAX);
	((A_u_char*)destData)[y*rowbytes / sizeof(A_u_char) + x * 4 + 2] = MIN(value.z * UCHAR_MAX, UCHAR_MAX);
	((A_u_char*)destData)[y*rowbytes / sizeof(A_u_char) + x * 4 + 3] = MIN(value.w * UCHAR_MAX, UCHAR_MAX);
}

static inline vec4 readVec4(const char* outgoingRowData, int x_new, int mode) {
	if(mode == 2)
		return read32bitVec4(outgoingRowData, x_new);
	else if(mode == 1)
		return read16bitVec4(outgoingRowData, x_new);
	else
		return read8bitVec4(outgoingRowData, x_new);
}

static inline void writeVec4(char* destData, int x, int y, int rowbytes, vec4 value, int mode) {
	if(mode==2)
		write32bitVec4(destData, x, y, rowbytes, value);
	else if(mode==1)
		write16bitVec4(destData, x, y, rowbytes, value);
	else
		write8bitVec4(destData, x, y, rowbytes, value);
}

/*
**
*/
static PF_Err Render(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_EffectWorld	*inputP,
	PF_ParamDef* params[],
	PF_LayerDef* output,
	bool smartRender,
	int mode)
{
	//TEMP
	if (in_data->current_time == 0)
		return PF_Err_NONE;

	KeyFrameManager::getInstance().isCpuProcessing = true;
#ifdef BETA_FAIL
	time_t time_ = time(NULL);

	if (time_ > BETA_FAIL_TIME) {
		return 0;
	}
#endif

	PF_LayerDef* outgoing = inputP;
	if (!smartRender)
		outgoing = &params[0]->u.ld;
	PF_LayerDef* dest = output;

	int width = output->width;
	int height = output->height;

	int samples = (int)round(params[MB_SAMPLES]->u.fs_d.value);
	//samples = max(1, samples);

	//TODO: TEMP!!!
	int cam1, cam2;
	float shutter = (int)round(params[MB_SHUTTER]->u.fs_d.value);

	float* fovs = (float*)malloc(sizeof(float)*samples);
	float* tinyplanets = (float*)malloc(sizeof(float)*samples);
	float* rectilinears = (float*)malloc(sizeof(float)*samples);
	float* rotmats = (float*)malloc(sizeof(float)*samples * 9);

	fillParamStructs(samples, shutter, in_data, params, cam1, cam2, rotmats, fovs, tinyplanets, rectilinears);
		
	const float* outgoingData = (const float*)outgoing->data;
	int inRowbytes = outgoing->rowbytes;

	char* destData = (char*)dest->data;
	int rowbytes = dest->rowbytes;

	float aspect = (float)width / (float)height;

	//#pragma loop(hint_parallel(0))
	//#pragma loop(ivdep)
	for (int y = 0; y < height;
		++y)
	{
		for (int x = 0; x < width; ++x)
		{
			vec4 accumValue = vec4(0, 0, 0, 0);
			for (int i = 0; i < samples; i++)
			{

				mat3 rotMat;
				memcpy(&rotMat[0], &(rotmats[i * 9]), sizeof(float) * 9);

				float fov = fovs[i];
				vec2 uv = vec2((float)x / width, (float)y / height);

				vec3 dir = vec3(0, 0, 0);
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
					const char* outgoingData_ = (const char*)outgoingData + y_new * inRowbytes;

					vec4 interpCol;

					vec4 outgoing = readVec4(outgoingData_, x_new, mode);

					float recipNewAlpha = 1.0f;

					interpCol = outgoing;

					accumValue += interpCol / (float)samples;
				}

				writeVec4(destData, x, y, rowbytes, accumValue, mode);
			}
			continue;
		}
	}

	free(rotmats);
	free(fovs);
	free(tinyplanets);
	free(rectilinears);
	

	return PF_Err_NONE;
}

static inline PF_Err DoRender(
	PF_InData		*in_data,
	PF_EffectWorld	*inputP,
	RenderData		*rendP,
	PF_OutData      *out_data,
	PF_LayerDef		*outputP,
	PF_ParamDef		*params[])
{
	PF_Err err = PF_Err_NONE;

	AEFX_SuiteScoper<PF_WorldSuite2> wsP(in_data, kPFWorldSuite, kPFWorldSuiteVersion2, out_data);
	if (!err) {
		PF_Point origin;
		PF_Rect areaR;
		origin.h = (A_short)(in_data->output_origin_x);
		origin.v = (A_short)(in_data->output_origin_y);
		areaR.top = areaR.left = 0;
		areaR.right = 1;
		areaR.bottom = (A_short)outputP->height;

		// do high bit depth rendering in Premiere Pro
		if (in_data->appl_id == 'PrMr') {
			// get the Premiere pixel format suite
			AEFX_SuiteScoper<PF_PixelFormatSuite1> pfS(in_data, kPFPixelFormatSuite, kPFPixelFormatSuiteVersion1, out_data);
			PrPixelFormat format = PrPixelFormat_BGRA_4444_8u;
			if (&pfS) {
				pfS->GetPixelFormat(outputP, &format);

				if (format == PrPixelFormat_BGRA_4444_8u) {
					return Render(in_data, out_data, inputP, params, outputP, false, 0);
				}
				else if (format == PrPixelFormat_BGRA_4444_32f) {
					return Render(in_data, out_data, inputP, params, outputP, false, 2);
				}
			}
		}
		else {
			// determine pixel format
			PF_PixelFormat format;
			wsP->PF_GetPixelFormat(outputP, &format);
			switch (format) {
			case PF_PixelFormat_ARGB128:
			{
				rendP->bitDepth = 32;
				return Render(in_data, out_data, inputP, params, outputP, true, 2);
			}
			break;
			case PF_PixelFormat_ARGB64:
			{
				rendP->bitDepth = 16;
				return Render(in_data, out_data, inputP, params, outputP, true, 1);
			}
			break;
			case PF_PixelFormat_ARGB32:
			{
				rendP->bitDepth = 8;
				return Render(in_data, out_data, inputP, params, outputP, true, 0);
			}
			break;
			default:
				err = PF_Err_BAD_CALLBACK_PARAM;
				break;
			}
		}
	}

	return err;
}

static PF_Err PreRender(
	PF_InData			*in_data,
	PF_OutData			*out_data,
	PF_PreRenderExtra	*extra)
{
	PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;
	PF_RenderRequest req = extra->input->output_request;
	PF_CheckoutResult in_result;
	ERR(extra->cb->checkout_layer(in_data->effect_ref, 0, 0,
		&req, in_data->current_time, in_data->time_step, in_data->time_scale, &in_result));
	//TODO: temp change
	extra->output->result_rect = in_result.result_rect;
	extra->output->max_result_rect = in_result.max_result_rect;

	PF_ParamDef* params[TOTAL_PARAM_NUM];
	// checkout the required params
	for (int i = 1; i < TOTAL_PARAM_NUM; i++) {
		params[i] = new PF_ParamDef;
		AEFX_CLR_STRUCT(*params[i]);
		PF_CHECKOUT_PARAM(in_data, i, in_data->current_time, in_data->time_step, in_data->time_scale, params[i]);
	}

	storeParamKeyframes(in_data, params, out_data);

	// Always check in, no matter what the error condition!
	for (int i = 1; i < NUM_PARAMS; i++) {
		ERR2(PF_CHECKIN_PARAM(in_data, params[i]));
		delete params[i];
	}

	return err;
}

static PF_Err DoNonSmartRender(PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output)
{
	if (in_data->appl_id == 'PrMr') {
		PF_EffectWorld *inputP = NULL, *outputP = NULL;
		PF_Err err = PF_Err_NONE;
		// checkout input & output layers
		inputP = &params[0]->u.ld;
		if (err) return err;

		// get the Premiere pixel format suite
		AEFX_SuiteScoper<PF_PixelFormatSuite1> pfS(in_data, kPFPixelFormatSuite, kPFPixelFormatSuiteVersion1, out_data);
		PrPixelFormat format = PrPixelFormat_BGRA_4444_8u;
		if (&pfS) {
			pfS->GetPixelFormat(output, &format);

			if (format == PrPixelFormat_BGRA_4444_8u) {
				return Render(in_data, out_data, inputP, params, output, false, 0);
			}
			else if (format == PrPixelFormat_BGRA_4444_32f) {
				return Render(in_data, out_data, inputP, params, output, false, 2);
			}
		}
	}
	return Render(in_data, out_data, NULL, params, output, false, 2);
}

static PF_Err SmartRender(
	PF_InData				*in_data,
	PF_OutData				*out_data,
	PF_SmartRenderExtra		*extra)
{
	PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

	PF_EffectWorld *inputP = NULL, *outputP = NULL;

	PF_ParamDef* params[TOTAL_PARAM_NUM];

	// checkout input & output layers
	ERR(extra->cb->checkout_layer_pixels(in_data->effect_ref, 0, &inputP));
	if (err) return err;

	ERR(extra->cb->checkout_output(in_data->effect_ref, &outputP));
	if (err) return err;

	// checkout the required params
	for (int i = 1; i < TOTAL_PARAM_NUM; i++) {
		params[i] = new PF_ParamDef;
		AEFX_CLR_STRUCT(*params[i]);
		PF_CHECKOUT_PARAM(in_data, i, in_data->current_time, in_data->time_step, in_data->time_scale, params[i]);
	}

	RenderData rd;
	rd.in_data = in_data;
	rd.inputP = inputP;
	rd.inOutput = outputP;
	rd.samp_pb.src = inputP;

	ERR(DoRender(in_data, inputP, &rd, out_data, outputP, params));

	// Always check in, no matter what the error condition!
	for (int i = 1; i < TOTAL_PARAM_NUM; i++) {
		ERR2(PF_CHECKIN_PARAM(in_data, params[i]));
		delete params[i];
	}
	return err;
}

static PF_Err FrameSetup(PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output) {


	storeParamKeyframes(in_data, params, out_data);


	return PF_Err_NONE;
}

static string intToStr(const int i)
{
	stringstream ss;
	ss << i;
	return ss.str();
}

static PF_Err About(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*outputP)
{
	PF_Err err = PF_Err_NONE;
	if (in_data->appl_id == 'PrMr') return err;
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	string desc = FX_DESCRIPTION;
	// ********************* aescripts licensing specific code start *********************
	string licString = lic::getLicenseDataAsString(lic::licenseData);
	if (!lic::licenseData.registered) licString += "\rDays since first use: " + intToStr(lic::licenseData.numberOfDaysSinceFirstStart);
	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg, "%s, v%d.%d\r%s\rRegistered to: %s",
		FX_NAME, PLUGIN_MAJOR_VERSION, PLUGIN_MINOR_VERSION, desc.c_str(), licString.c_str());
	// ********************* aescripts licensing specific code end *********************
	return err;
}

static PF_Err Register(
	PF_InData		*in_data,
	PF_OutData		*out_data)
{
	PF_Err err = PF_Err_NONE;
	// ********************* aescripts licensing specific code start *********************
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	int reg_result = lic::showRegistrationDialog(&suites, lic::licenseData, in_data->appl_id != 'PrMr');
	// ********************* aescripts licensing specific code end *********************
	return err;
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
	AEGP_SuiteHandler suites(in_data->pica_basicP);

	PF_Err err = PF_Err_NONE;
	// ********************* aescripts licensing specific code start *********************
	if (lic::checkLicenseAE(inCmd, in_data, out_data, lic::licenseData) != 0) return err;
	// ********************* aescripts licensing specific code end *********************

	if (lic::licenseData.registered) {
		KeyFrameManager::getInstance().isRegistered = true;
	}
	else {
		KeyFrameManager::getInstance().isRegistered = false;
	}

	if (in_data->appl_id != 'PrMr') {
		KeyFrameManager::getInstance().isAE = true;
	}


	//grlic::testLicenseCheck();
	//grlic::getLicenseStoreDir();

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
	{
#ifdef BETA_FAIL
		time_t time_ = time(NULL);

		if (time_ > BETA_FAIL_TIME) {
			sprintf(out_data->return_msg, "BETA VERSION EXPIRED!");
			err = 1;
		}
#endif
		break;
	}
	case PF_Cmd_SEQUENCE_RESETUP:
	{
#ifdef BETA_FAIL
		time_t time_ = time(NULL);

		if (time_ > BETA_FAIL_TIME) {
			sprintf(out_data->return_msg, "BETA VERSION EXPIRED!");
			err = 1;
		}
#endif
		break;
	}
	case PF_Cmd_FRAME_SETUP:
		err = FrameSetup(in_data, out_data, params, inOutput);
		break;
	case PF_Cmd_GET_FLATTENED_SEQUENCE_DATA:
		err = 0;
		break;
	case PF_Cmd_RENDER:
	{
		//if (!lic::licenseData.registered) {
		//	PF_SPRINTF(out_data->return_msg, "not registered!");
		//	return err;
		//}
		err = DoNonSmartRender(in_data, out_data, params, inOutput);
		break;
	}
	case PF_Cmd_SMART_PRE_RENDER:
	{
		err = PreRender(in_data, out_data, reinterpret_cast<PF_PreRenderExtra*>(extra));
		break;
	}
	case PF_Cmd_SMART_RENDER:
	{
		//if (!lic::licenseData.registered) {
		//	PF_SPRINTF(out_data->return_msg, "not registered!");
		//	return err;
		//}
		err = SmartRender(in_data, out_data, reinterpret_cast<PF_SmartRenderExtra*>(extra));
		break;
	}
	case PF_Cmd_ABOUT:
	{
		err = About(in_data, out_data, params, inOutput);
		break;
	}
	case PF_Cmd_DO_DIALOG:
	{
		err = Register(in_data, out_data);
		break;
	}
	return err;
	}
	return err;
}
