#pragma once

#include "AEConfig.h"

#include "PrSDKTypes.h"
#include "AE_Effect.h"
#include "A.h"
#include "AE_Macros.h"
#include "AEFX_SuiteHandlerTemplate.h"
#include "Param_Utils.h"
#include "Smart_Utils.h"

#include "SDK_CrossDissolve.h"
#include <math.h>
#include "MathUtil.h"

static CameraParams mainCameraParams(PF_ParamDef* params[]) {
	CameraParams outParams;

	outParams.pitch = params[MAIN_CAMERA_PITCH]->u.fs_d.value;
	outParams.yaw = params[MAIN_CAMERA_YAW]->u.fs_d.value;
	outParams.roll = params[MAIN_CAMERA_ROLL]->u.fs_d.value;
	outParams.fov = params[MAIN_CAMERA_FOV]->u.fs_d.value;
	outParams.tinyplanet = params[MAIN_CAMERA_TINYPLANET]->u.fs_d.value;
	outParams.rectilinear = params[MAIN_CAMERA_RECTILINEAR]->u.fs_d.value;

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

static float getMainFov(PF_ParamDef* params[]) {
	return params[MAIN_CAMERA_FOV]->u.fs_d.value;
}

static float getMainTinyplanet(PF_ParamDef* params[]) {
	return params[MAIN_CAMERA_TINYPLANET]->u.fs_d.value;
}

static float getMainRectilinear(PF_ParamDef* params[]) {
	return params[MAIN_CAMERA_RECTILINEAR]->u.fs_d.value;
}
