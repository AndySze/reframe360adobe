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
#pragma once

#define BETA_FAIL
#define BETA_FAIL_TIME 1546301340

#ifndef SDK_CROSSDISSOLVE_H
#define SDK_CROSSDISSOLVE_H

// name and description for the effect
#define FX_NAME "Reframe360"
#define FX_DESCRIPTION "This is a plugin that allows you to reframe your 360 degree footage."
#define FX_CATEGORY "visuality"
#define FX_UNIQUEID "at.visuality.Reframe360"

#include "AEConfig.h"

#include "PrSDKTypes.h"
#include "AE_Effect.h"
#include "A.h"
#include "AE_Macros.h"
#include "AEFX_SuiteHandlerTemplate.h"
#include "Param_Utils.h"
#include "Smart_Utils.h"

#include "CameraUtil.h"

#include "PrSDKAESupport.h"

#include <math.h>

 #include "Licensing.h"

struct RenderData
{
	PF_InData *in_data;
	PF_EffectWorld *inputP;
	PF_LayerDef *inOutput;
	PF_SampPB samp_pb;
	A_long bitDepth;
};

/*
**
*/
enum
{
	SDK_CROSSDISSOLVE_INPUT = 0,
	SDK_CROSSDISSOLVE_FLIP,
	SDK_CROSSDISSOLVE_NUM_PARAMS
};

/*
**
*/
#define	PROGRESS_MIN_VALUE		0.0f
#define	PROGRESS_MAX_VALUE		100.0f
#define	PROGRESS_MIN_SLIDER		0.0f
#define	PROGRESS_MAX_SLIDER		100.0f
#define	PROGRESS_DFLT			0.0f

/*
**
*/
#define	PLUGIN_MAJOR_VERSION		1
#define	PLUGIN_MINOR_VERSION		0
#define	PLUGIN_BUG_VERSION			0
#define	PLUGIN_STAGE_VERSION		PF_Stage_DEVELOP
#define	PLUGIN_BUILD_VERSION		0

enum
{
	MAIN_INPUT=0,

	MB_GRP_ID,
	MB_SAMPLES,
	MB_SHUTTER,
	MB_GRP_END_ID,

	MAIN_CAMERA_GRP_ID,
	MAIN_CAMERA_PITCH,
	MAIN_CAMERA_YAW,
	MAIN_CAMERA_ROLL,
	MAIN_CAMERA_FOV,
	MAIN_CAMERA_GRP_END_ID,

	BLEND_GRP_ID,
	AUX_CAM_SEQUENCE,
	AUX_ACCELERATION,
	BLEND_GRP_END_ID,

	CAM_SELECTION_GRP_ID,
	ACTIVE_AUX_CAMERA_SELECTOR,
	FORCE_AUX_DISPLAY,
	CAM_SELECTION_GRP_END_ID,

	AUX_CAMERA_GRP_ID,
	AUX_CAMERA_COPY_BTN_ID,
	AUX_CAMERA_PASTE_BTN_ID,
	AUX_CAMERA_PITCH,
	AUX_CAMERA_YAW,
	AUX_CAMERA_ROLL,
	AUX_CAMERA_FOV,
	AUX_CAMERA_TINYPLANET,
	AUX_CAMERA_RECTILINEAR,
	AUX_CAMERA_GRP_END_ID,
	NUM_PARAMS
};

#define AUX_PARAM_NUM 6

/*
**
*/
#define	CAMERA_MIN_VALUE		1
#define	CAMERA_MAX_VALUE		20
#define	CAMERA_MIN_SLIDER		1
#define	CAMERA_MAX_SLIDER		10
#define	CAMERA_DFLT				1

#define	MAIN_CAMERA_PITCH_MIN_VALUE			-900.0f
#define	MAIN_CAMERA_PITCH_MAX_VALUE			900.0f
#define	MAIN_CAMERA_PITCH_MIN_SLIDER		-90.0f
#define	MAIN_CAMERA_PITCH_MAX_SLIDER		90.0f
#define	MAIN_CAMERA_PITCH_DFLT				0

#define	MAIN_CAMERA_YAW_MIN_VALUE			-1800.0
#define	MAIN_CAMERA_YAW_MAX_VALUE			1800.0
#define	MAIN_CAMERA_YAW_MIN_SLIDER			-180.0
#define	MAIN_CAMERA_YAW_MAX_SLIDER			180.0
#define	MAIN_CAMERA_YAW_DFLT				0

#define	MAIN_CAMERA_ROLL_MIN_VALUE			-1800.0
#define	MAIN_CAMERA_ROLL_MAX_VALUE			1800.0
#define	MAIN_CAMERA_ROLL_MIN_SLIDER			-180.0
#define	MAIN_CAMERA_ROLL_MAX_SLIDER			180.0
#define	MAIN_CAMERA_ROLL_DFLT				0

#define	MAIN_CAMERA_FOV_MIN_VALUE			0.1
#define	MAIN_CAMERA_FOV_MAX_VALUE			10.0
#define	MAIN_CAMERA_FOV_MIN_SLIDER			0.3
#define	MAIN_CAMERA_FOV_MAX_SLIDER			3.0
#define	MAIN_CAMERA_FOV_DFLT				1.0

#define	MB_SAMPLES_MIN_VALUE			1.0
#define	MB_SAMPLES_MAX_VALUE			512
#define	MB_SAMPLES_MIN_SLIDER			1
#define	MB_SAMPLES_MAX_SLIDER			64
#define	MB_SAMPLES_DFLT				1

#define	MB_SHUTTER_MIN_VALUE			0
#define	MB_SHUTTER_MAX_VALUE			3
#define	MB_SHUTTER_MIN_SLIDER			0
#define	MB_SHUTTER_MAX_SLIDER			1
#define	MB_SHUTTER_DFLT					.5

#define	AUX_ACCELERATION_MIN_VALUE			1.0
#define	AUX_ACCELERATION_MAX_VALUE			20.0
#define	AUX_ACCELERATION_MIN_SLIDER			0.0
#define	AUX_ACCELERATION_MAX_SLIDER			7.0
#define	AUX_ACCELERATION_DFLT				3.0

#define	AUX_CAMERA_PITCH_MIN_VALUE			-1800.0
#define	AUX_CAMERA_PITCH_MAX_VALUE			1800.0
#define	AUX_CAMERA_PITCH_MIN_SLIDER		-90.0
#define	AUX_CAMERA_PITCH_MAX_SLIDER		90.0
#define	AUX_CAMERA_PITCH_DFLT				0

#define	AUX_CAMERA_YAW_MIN_VALUE			-1800.0
#define	AUX_CAMERA_YAW_MAX_VALUE			1800.0
#define	AUX_CAMERA_YAW_MIN_SLIDER			-180.0
#define	AUX_CAMERA_YAW_MAX_SLIDER			180.0
#define	AUX_CAMERA_YAW_DFLT				0

#define	AUX_CAMERA_ROLL_MIN_VALUE			-1800.0
#define	AUX_CAMERA_ROLL_MAX_VALUE			1800.0
#define	AUX_CAMERA_ROLL_MIN_SLIDER			-180.0
#define	AUX_CAMERA_ROLL_MAX_SLIDER			180.0
#define	AUX_CAMERA_ROLL_DFLT				0

#define	AUX_CAMERA_FOV_MIN_VALUE			0.01
#define	AUX_CAMERA_FOV_MAX_VALUE			10.0
#define	AUX_CAMERA_FOV_MIN_SLIDER			0.15
#define	AUX_CAMERA_FOV_MAX_SLIDER			5.0
#define	AUX_CAMERA_FOV_DFLT				1.0

#define	AUX_CAMERA_TINYPLANET_MIN_VALUE			0.0
#define	AUX_CAMERA_TINYPLANET_MAX_VALUE			1.0
#define	AUX_CAMERA_TINYPLANET_MIN_SLIDER			0.0
#define	AUX_CAMERA_TINYPLANET_MAX_SLIDER			1.0
#define	AUX_CAMERA_TINYPLANET_DFLT					0.0

#define	AUX_CAMERA_RECTILINEAR_MIN_VALUE			0.0
#define	AUX_CAMERA_RECTILINEAR_MAX_VALUE			1.0
#define	AUX_CAMERA_RECTILINEAR_MIN_SLIDER			0.0
#define	AUX_CAMERA_RECTILINEAR_MAX_SLIDER			1.0
#define	AUX_CAMERA_RECTILINEAR_DFLT				0.0

#define TOTAL_PARAM_NUM (NUM_PARAMS - AUX_PARAM_NUM + AUX_PARAM_NUM * CAMERA_MAX_VALUE)


typedef struct  CameraParams {
	float pitch = MAIN_CAMERA_PITCH_DFLT;
	float yaw = MAIN_CAMERA_YAW_DFLT;
	float roll = MAIN_CAMERA_ROLL_DFLT;
	float fov = AUX_CAMERA_FOV_DFLT;
	float tinyplanet = AUX_CAMERA_TINYPLANET_DFLT;
	float rectilinear = AUX_CAMERA_RECTILINEAR_DFLT;
}CameraParams;

typedef struct ParamSet {
	int id;
	CameraParams mainCamParams;

	int activeCamera = CAMERA_DFLT;
	int camera1 = 1;
	int camera2 = 2;
	float blend = 0;
	float acceleration = 3.0f;

	bool forceActiveAuxCamera = false;

	CameraParams auxCamParams[CAMERA_MAX_VALUE];

}ParamSet;

#endif
