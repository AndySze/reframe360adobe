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


#include "SDK_CrossDissolve.h"
#include <map>
#include "ParamUtil.h"

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

	int num_params = 0;

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(
		"Main Pitch",
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
		"Main Yaw",
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
		"Main Roll",
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
		"Main Zoom",
		MAIN_CAMERA_FOV_MIN_VALUE,
		MAIN_CAMERA_FOV_MAX_VALUE,
		MAIN_CAMERA_FOV_MIN_SLIDER,
		MAIN_CAMERA_FOV_MAX_SLIDER,
		MAIN_CAMERA_FOV_DFLT,
		PF_Precision_HUNDREDTHS,
		PF_ValueDisplayFlag_NONE,
		0,
		MAIN_CAMERA_FOV
	);
	num_params++;

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(
		"Main Tiny Planet",
		MAIN_CAMERA_TINYPLANET_MIN_VALUE,
		MAIN_CAMERA_TINYPLANET_MAX_VALUE,
		MAIN_CAMERA_TINYPLANET_MIN_SLIDER,
		MAIN_CAMERA_TINYPLANET_MAX_SLIDER,
		MAIN_CAMERA_TINYPLANET_DFLT,
		PF_Precision_HUNDREDTHS,
		PF_ValueDisplayFlag_NONE,
		0,
		MAIN_CAMERA_TINYPLANET
	);
	num_params++;

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(
		"Main Rectify",
		MAIN_CAMERA_RECTILINEAR_MIN_VALUE,
		MAIN_CAMERA_RECTILINEAR_MAX_VALUE,
		MAIN_CAMERA_RECTILINEAR_MIN_SLIDER,
		MAIN_CAMERA_RECTILINEAR_MAX_SLIDER,
		MAIN_CAMERA_RECTILINEAR_DFLT,
		PF_Precision_HUNDREDTHS,
		PF_ValueDisplayFlag_NONE,
		0,
		MAIN_CAMERA_RECTILINEAR
	);
	num_params++;

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(
		"Aux Camera",
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

	PF_ADD_FLOAT_SLIDERX(
		"Aux Camera",
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
		"Aux Camera",
		CAMERA_MIN_VALUE,
		CAMERA_MAX_VALUE,
		CAMERA_MIN_SLIDER,
		CAMERA_MAX_SLIDER,
		CAMERA_DFLT,
		PF_Precision_INTEGER,
		PF_ValueDisplayFlag_NONE,
		0,
		AUX_CAMERA2
	);
	num_params++;

	AEFX_CLR_STRUCT(def);

	PF_ADD_CHECKBOXX(
		"Show Aux Camera",
		false,
		0,
		FORCE_AUX_DISPLAY
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
		MB_SAMPLES_MIN_VALUE,
		MB_SAMPLES_MAX_VALUE,
		MB_SAMPLES_MIN_SLIDER,
		MB_SAMPLES_MAX_SLIDER,
		MB_SAMPLES_DFLT,
		PF_Precision_TENTHS,
		PF_ValueDisplayFlag_NONE,
		0,
		AUX_ACCELERATION
	);
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

	num_params += addAuxParams(in_data, false, 0);

	PF_ADD_TOPIC(
		"Aux Cameras",
		AUX_CAMERA_GRP_ID
	);

	for (int i = 1; i <= CAMERA_MAX_VALUE; i++) {
		num_params += addAuxParams(in_data, true, i);
	}

	PF_END_TOPIC(
		AUX_CAMERA_GRP_ID
	);

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

	ParamSet* paramSet = (ParamSet*)in_data->sequence_data;

	PF_UserChangedParamExtra* param_extra = (PF_UserChangedParamExtra*)extra;

	int selectedCam = getSelectedCamera(params);

	if (param_extra->param_index == ACTIVE_AUX_CAMERA_SELECTOR) {

		CameraParams selectedParams = paramSet->auxCamParams[selectedCam-1];

		params[AUX_CAMERA_PITCH]->u.fs_d.value = selectedParams.pitch;
		params[AUX_CAMERA_PITCH]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_YAW]->u.fs_d.value = selectedParams.yaw;
		params[AUX_CAMERA_YAW]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_ROLL]->u.fs_d.value = selectedParams.roll;
		params[AUX_CAMERA_ROLL]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_FOV]->u.fs_d.value = selectedParams.fov;
		params[AUX_CAMERA_FOV]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_TINYPLANET]->u.fs_d.value = selectedParams.tinyplanet;
		params[AUX_CAMERA_TINYPLANET]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;

		params[AUX_CAMERA_RECTILINEAR]->u.fs_d.value = selectedParams.rectilinear;
		params[AUX_CAMERA_RECTILINEAR]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
	}
	else {
		float changedParam = params[param_extra->param_index]->u.fs_d.value;

		CameraParams selectedParams = paramSet->auxCamParams[selectedCam-1];

		switch (param_extra->param_index) {
		case AUX_CAMERA_PITCH:
			selectedParams.pitch = changedParam;
			break;
		case AUX_CAMERA_YAW:
			selectedParams.yaw = changedParam;
			break;
		case AUX_CAMERA_ROLL:
			selectedParams.roll = changedParam;
			break;
		case AUX_CAMERA_FOV:
			selectedParams.fov = changedParam;
			break;
		case AUX_CAMERA_TINYPLANET:
			selectedParams.tinyplanet = changedParam;
			break;
		case AUX_CAMERA_RECTILINEAR:
			selectedParams.rectilinear = changedParam;
			break;
		default:
			break;
		}

		paramSet->auxCamParams[selectedCam-1] = selectedParams;
	}

	return PF_Err_NONE;
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

	PF_LayerDef* outgoing = &params[0]->u.ld;
	PF_LayerDef* dest = output;

	const char* outgoingData = (const char*)outgoing->data;

	char* destData = (char*)dest->data;

	float pitch = params[MAIN_CAMERA_PITCH]->u.fs_d.value;

	if (params[SDK_CROSSDISSOLVE_FLIP]->u.bd.value)
	{
		destData += (output->height - 1) * dest->rowbytes;
		dest->rowbytes = -dest->rowbytes;
	}

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
	}

	return PF_Err_NONE;
}

static PF_Err SequenceSetup(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output) {

	ParamSet* seqParamSet = new ParamSet();

	out_data->sequence_data = (PF_Handle)seqParamSet;
	

	return PF_Err_NONE;
}

static PF_Err Flatten(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output) {

	float pitch = params[MAIN_CAMERA_PITCH]->u.fs_d.value;


	return PF_Err_NONE;
}

static PF_Err Unflatten(
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
	case PF_Cmd_SEQUENCE_RESETUP:
		break;
	case PF_Cmd_SEQUENCE_FLATTEN:
		break;
	case PF_Cmd_GET_FLATTENED_SEQUENCE_DATA:
		break;
	case PF_Cmd_RENDER:
		err = Render(in_data, out_data, params, inOutput);
		break;
	}
	return err;
}