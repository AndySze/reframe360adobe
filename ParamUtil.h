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
#include "KeyFrameManager.h"


#pragma optimize( "", off )
double static findNeighbourKeyframeTime_CPU_Premiere(int paramID, PF_InData* in_data, bool backwards) {
	PrTime time = in_data->current_time;
	PrTime timeStep = in_data->time_step;
	if (backwards)
		timeStep = -timeStep;

	PF_ParamDef	def;
	AEFX_CLR_STRUCT(def);

	PF_CHECKOUT_PARAM(in_data, paramID, time, timeStep, in_data->time_scale, &def);
	float val = def.u.fs_d.value;
	PF_CHECKIN_PARAM(in_data, &def);

	AEFX_CLR_STRUCT(def);
	time += timeStep;
	PF_CHECKOUT_PARAM(in_data, paramID, time, timeStep, in_data->time_scale, &def);
	float val_nextFrame = def.u.fs_d.value;
	PF_CHECKIN_PARAM(in_data, &def);

	float ref_delta = val_nextFrame - val;
	if (ref_delta == 0)
		return time - timeStep;

	float new_delta = FLT_MAX;

	double delta_diff = 0;

	float currentVal = val_nextFrame;
	float nextVal;
	while (new_delta != 0 && delta_diff < 0.0001f) {
		time += timeStep;;
		PF_CHECKOUT_PARAM(in_data, paramID, time, timeStep, in_data->time_scale, &def);
		nextVal = def.u.fs_d.value;
		PF_CHECKIN_PARAM(in_data, &def);

		new_delta = nextVal - currentVal;
		delta_diff = abs(new_delta - ref_delta);
		currentVal = nextVal;
	}
	return time - timeStep;;
}
#pragma optimize( "", on )

static CameraParams mainCameraParams(PF_ParamDef* params[]) {
	CameraParams outParams;

	outParams.pitch = params[KeyFrameManager::getInstance().idToIndex[MAIN_CAMERA_PITCH]]->u.fs_d.value;
	outParams.yaw = params[KeyFrameManager::getInstance().idToIndex[MAIN_CAMERA_YAW]]->u.fs_d.value;
	outParams.roll = params[KeyFrameManager::getInstance().idToIndex[MAIN_CAMERA_ROLL]]->u.fs_d.value;

	return outParams;
}

static CameraParams activeAuxCameraParams(PF_ParamDef* params[]) {
	CameraParams outParams;

	int activeCam = (int)round(params[KeyFrameManager::getInstance().idToIndex[ACTIVE_AUX_CAMERA_SELECTOR]]->u.fs_d.value);

	outParams.pitch = params[KeyFrameManager::getInstance().idToIndex[AUX_CAMERA_PITCH]]->u.fs_d.value;
	outParams.yaw = params[KeyFrameManager::getInstance().idToIndex[AUX_CAMERA_YAW]]->u.fs_d.value;
	outParams.roll = params[KeyFrameManager::getInstance().idToIndex[AUX_CAMERA_ROLL]]->u.fs_d.value;
	outParams.fov = params[KeyFrameManager::getInstance().idToIndex[AUX_CAMERA_FOV]]->u.fs_d.value;
	outParams.tinyplanet = params[KeyFrameManager::getInstance().idToIndex[AUX_CAMERA_TINYPLANET]]->u.fs_d.value;
	outParams.rectilinear = params[KeyFrameManager::getInstance().idToIndex[AUX_CAMERA_RECTILINEAR]]->u.fs_d.value;

	return outParams;
}


static int getSelectedCamera(PF_ParamDef* params[]) {
	int activeCam = (int)round(params[KeyFrameManager::getInstance().idToIndex[ACTIVE_AUX_CAMERA_SELECTOR]]->u.fs_d.value);
	return activeCam;
}


static int auxParamId(int baseId, int camId) {
	int refID = AUX_CAMERA_PITCH;
	int diff = baseId - refID;
	return refID + camId * AUX_PARAM_NUM + diff + (camId < 1 ? 0 : 1);
}


static int addAuxParams(PF_InData* in_data, bool hidden, int camera, int num_params) {
	PF_ParamDef	def;

	int refID = AUX_CAMERA_PITCH;

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_SUPERVISE;

	if (hidden) {
		def.ui_flags = PF_PUI_INVISIBLE;
	}
	PF_ADD_FLOAT_SLIDER(
		"Pitch",
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
	KeyFrameManager::getInstance().idToIndex[auxParamId(AUX_CAMERA_PITCH, camera)] = num_params;
	num_params++;

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_SUPERVISE;
	
	if (hidden) {
		def.ui_flags = PF_PUI_INVISIBLE;
	}
	PF_ADD_FLOAT_SLIDER(
		"Yaw",
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
	KeyFrameManager::getInstance().idToIndex[auxParamId(AUX_CAMERA_YAW, camera)] = num_params;
	num_params++;

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_SUPERVISE;
	
	if (hidden) {
		def.ui_flags = PF_PUI_INVISIBLE;
	}
	PF_ADD_FLOAT_SLIDER(
		"Roll",
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
	KeyFrameManager::getInstance().idToIndex[auxParamId(AUX_CAMERA_ROLL, camera)] = num_params;
	num_params++;

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_SUPERVISE;

	if (hidden) {
		def.ui_flags = PF_PUI_INVISIBLE;
	}
	PF_ADD_FLOAT_SLIDER(
		"Zoom",
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
	KeyFrameManager::getInstance().idToIndex[auxParamId(AUX_CAMERA_FOV, camera)] = num_params;
	num_params++;

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_SUPERVISE;

	if (hidden) {
		def.ui_flags = PF_PUI_INVISIBLE;
	}
	PF_ADD_FLOAT_SLIDER(
		"Tiny Planet",
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
	KeyFrameManager::getInstance().idToIndex[auxParamId(AUX_CAMERA_TINYPLANET, camera)] = num_params;
	num_params++;

	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_SUPERVISE;

	if (hidden) {
		def.ui_flags = PF_PUI_INVISIBLE;
	}
	PF_ADD_FLOAT_SLIDER(
		"Rectify",
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
	KeyFrameManager::getInstance().idToIndex[auxParamId(AUX_CAMERA_RECTILINEAR, camera)] = num_params;
	num_params++;

	return num_params;
}