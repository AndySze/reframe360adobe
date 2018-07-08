#pragma once

#include "AEConfig.h"

#include "PrSDKTypes.h"
#include "AE_Effect.h"
#include "A.h"
#include "AE_Macros.h"
#include "AEFX_SuiteHandlerTemplate.h"
#include "Param_Utils.h"
#include "Smart_Utils.h"

#include "PrGPUFilterModule.h"
#include "Reframe360.h"
#include <math.h>
#include "MathUtil.h"

static CameraParams mainCameraParams(PF_ParamDef* params[]) {
	CameraParams outParams;

	outParams.pitch = params[MAIN_CAMERA_PITCH]->u.fs_d.value;
	outParams.yaw = params[MAIN_CAMERA_YAW]->u.fs_d.value;
	outParams.roll = params[MAIN_CAMERA_ROLL]->u.fs_d.value;

	return outParams;
}

static CameraParams activeAuxCameraParams(PF_ParamDef* params[]) {
	CameraParams outParams;

	int activeCam = (int)round(params[ACTIVE_AUX_CAMERA_SELECTOR]->u.fs_d.value);

	outParams.pitch = params[AUX_CAMERA_PITCH]->u.fs_d.value;
	outParams.yaw = params[AUX_CAMERA_YAW]->u.fs_d.value;
	outParams.roll = params[AUX_CAMERA_ROLL]->u.fs_d.value;
	outParams.fov = params[AUX_CAMERA_FOV]->u.fs_d.value;
	outParams.tinyplanet = params[AUX_CAMERA_TINYPLANET]->u.fs_d.value;
	outParams.rectilinear = params[AUX_CAMERA_RECTILINEAR]->u.fs_d.value;

	return outParams;
}


static int getSelectedCamera(PF_ParamDef* params[]) {
	int activeCam = (int)round(params[ACTIVE_AUX_CAMERA_SELECTOR]->u.fs_d.value);
	return activeCam;
}

static int getCamera1(PF_ParamDef* params[]) {
	int cam = (int)round(params[AUX_CAMERA1]->u.fs_d.value);
	return cam;
}

static int getCamera2(PF_ParamDef* params[]) {
	int cam = (int)round(params[AUX_CAMERA2]->u.fs_d.value);
	return cam;
}

static float getCameraBlendRaw(PF_ParamDef* params[]) {
	return params[AUX_BLEND]->u.fs_d.value;
}

static float getCameraBlendAccel(PF_ParamDef* params[]) {
	return params[AUX_ACCELERATION]->u.fs_d.value;
}

static float getCameraBlend(PF_ParamDef* params[]) {

	float accel = getCameraBlendAccel(params);
	float blend = getCameraBlendRaw(params);

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

static float getMainPitch(PF_ParamDef* params[]) {
	return params[MAIN_CAMERA_PITCH]->u.fs_d.value;
}
static float getMainYaw(PF_ParamDef* params[]) {
	return params[MAIN_CAMERA_YAW]->u.fs_d.value;
}

static float getMainRoll(PF_ParamDef* params[]) {
	return params[MAIN_CAMERA_ROLL]->u.fs_d.value;
}

static int auxParamId(int baseId, int camId) {
	int refID = AUX_CAMERA_PITCH;
	int diff = baseId - refID;
	return refID + camId * AUX_PARAM_NUM + diff;
}

static int addAuxParams(PF_InData* in_data, bool hidden, int camera) {
	PF_ParamDef	def;
	int num_params = 0;

	int refID = AUX_CAMERA_PITCH;

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_SUPERVISE;

	if (hidden) {
		def.ui_flags = PF_PUI_INVISIBLE;
	}
	PF_ADD_FLOAT_SLIDER(
		"Aux Pitch",
		AUX_CAMERA_PITCH_MIN_VALUE,
		AUX_CAMERA_PITCH_MAX_VALUE,
		AUX_CAMERA_PITCH_MIN_SLIDER,
		AUX_CAMERA_PITCH_MAX_SLIDER,
		0,
		AUX_CAMERA_PITCH_DFLT,
		PF_Precision_TENTHS,
		PF_ValueDisplayFlag_NONE,
		PF_ParamFlag_SUPERVISE,
		auxParamId(AUX_CAMERA_PITCH, camera)
	);
	num_params++;

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_SUPERVISE;
	
	if (hidden) {
		def.ui_flags = PF_PUI_INVISIBLE;
	}
	PF_ADD_FLOAT_SLIDER(
		"Aux Yaw",
		AUX_CAMERA_YAW_MIN_VALUE,
		AUX_CAMERA_YAW_MAX_VALUE,
		AUX_CAMERA_YAW_MIN_SLIDER,
		AUX_CAMERA_YAW_MAX_SLIDER,
		0,
		AUX_CAMERA_YAW_DFLT,
		PF_Precision_TENTHS,
		PF_ValueDisplayFlag_NONE,
		PF_ParamFlag_SUPERVISE,
		auxParamId(AUX_CAMERA_YAW, camera)
	);
	num_params++;

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_SUPERVISE;
	
	if (hidden) {
		def.ui_flags = PF_PUI_INVISIBLE;
	}
	PF_ADD_FLOAT_SLIDER(
		"Aux Roll",
		AUX_CAMERA_ROLL_MIN_VALUE,
		AUX_CAMERA_ROLL_MAX_VALUE,
		AUX_CAMERA_ROLL_MIN_SLIDER,
		AUX_CAMERA_ROLL_MAX_SLIDER,
		0,
		AUX_CAMERA_ROLL_DFLT,
		PF_Precision_TENTHS,
		PF_ValueDisplayFlag_NONE,
		PF_ParamFlag_SUPERVISE,
		auxParamId(AUX_CAMERA_ROLL, camera)
	);
	num_params++;

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_SUPERVISE;

	if (hidden) {
		def.ui_flags = PF_PUI_INVISIBLE;
	}
	PF_ADD_FLOAT_SLIDER(
		"Aux Zoom",
		AUX_CAMERA_FOV_MIN_VALUE,
		AUX_CAMERA_FOV_MAX_VALUE,
		AUX_CAMERA_FOV_MIN_SLIDER,
		AUX_CAMERA_FOV_MAX_SLIDER,
		0,
		AUX_CAMERA_FOV_DFLT,
		PF_Precision_TENTHS,
		PF_ValueDisplayFlag_NONE,
		PF_ParamFlag_SUPERVISE,
		auxParamId(AUX_CAMERA_FOV, camera)
	);

	num_params++;

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_SUPERVISE;

	if (hidden) {
		def.ui_flags = PF_PUI_INVISIBLE;
	}
	PF_ADD_FLOAT_SLIDER(
		"Aux Tiny Planet",
		AUX_CAMERA_TINYPLANET_MIN_VALUE,
		AUX_CAMERA_TINYPLANET_MAX_VALUE,
		AUX_CAMERA_TINYPLANET_MIN_SLIDER,
		AUX_CAMERA_TINYPLANET_MAX_SLIDER,
		0,
		AUX_CAMERA_TINYPLANET_DFLT,
		PF_Precision_TENTHS,
		PF_ValueDisplayFlag_NONE,
		PF_ParamFlag_SUPERVISE,
		auxParamId(AUX_CAMERA_TINYPLANET, camera)
	);

	num_params++;

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_SUPERVISE;

	if (hidden) {
		def.ui_flags = PF_PUI_INVISIBLE;
	}
	PF_ADD_FLOAT_SLIDER(
		"Aux Rectify",
		AUX_CAMERA_RECTILINEAR_MIN_VALUE,
		AUX_CAMERA_RECTILINEAR_MAX_VALUE,
		AUX_CAMERA_RECTILINEAR_MIN_SLIDER,
		AUX_CAMERA_RECTILINEAR_MAX_SLIDER,
		0,
		AUX_CAMERA_RECTILINEAR_DFLT,
		PF_Precision_TENTHS,
		PF_ValueDisplayFlag_NONE,
		PF_ParamFlag_SUPERVISE,
		auxParamId(AUX_CAMERA_RECTILINEAR, camera)
	);
	num_params++;

	return num_params;
}