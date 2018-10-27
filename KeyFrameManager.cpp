#include "KeyFrameManager.h"
#include "ParamUtil.h"

prSuiteError KeyFrameManager::getNextKeyFrameTime(int param, A_long inTime, A_long *outTime) {
	return _sequenceKeyFrames.getNextKeyFrameTime(param, inTime, outTime);
}

void KeyFrameManager::addKeyFrame(int param, A_long time, int value) {
	_sequenceKeyFrames.addKeyFrame(param, time, (float)value);
}

float KeyFrameManager::getKeyFrameValue(int param, A_long time) {
	return _sequenceKeyFrames.getKeyFrameValue(param, time);
}



void KeyFrameManager::beginKeyNewKeyframeData() {
    _sequenceKeyFrames.reset();
}
void KeyFrameManager::setRecordingKeyframe(int param, A_long time, float value) {
    _currentRecordingKeyFrames.addKeyFrame(param, time, (float)value);
}

void KeyFrameManager::stopRemoteRecording(){
    _isRecording = false;
}

bool KeyFrameManager::isRecording(){
    return _isRecording;
}

KeyFrameData KeyFrameManager::getCurrentRecordingKeyframeData() {
    return _currentRecordingKeyFrames;
}

void KeyFrameManager::beginKeyNewRemoteRecording() {
    _currentRecordingKeyFrames.reset();
    _isRecording = true;

}

void KeyFrameManager::setCurrentAETime(A_long time) {
	_currentAETime = time;
}

A_long KeyFrameManager::getCurrentAETime() {
	return _currentAETime;
}

int KeyFrameManager::getCamToCopy() {
	return _camToCopy;
}

void KeyFrameManager::setCamToCopy(int cam) {
	_camToCopy = cam;
}


PrParam KeyFrameManager::GetParam(
	csSDK_int32 nodeID,
	PrSDKVideoSegmentSuite* videoSegmentSuite,
	csSDK_int32 inIndex,
	PrTime inTime)
{
	inIndex -= 1; // GPU filters do not include the input frame

	PrParam param = {};
	videoSegmentSuite->GetParam(nodeID, inIndex, inTime, &param);
	return param;
}

prSuiteError KeyFrameManager::GetNextKeyframeTime(
	csSDK_int32 inVideoNodeID,
	PrSDKVideoSegmentSuite* videoSegmentSuite,
	csSDK_int32 inIndex,
	PrTime inTime,
	PrTime* outKeyframeTime,
	csSDK_int32* outKeyframeInterpolationMode) {

	if (!KeyFrameManager::getInstance().isAE) {
		prSuiteError error = videoSegmentSuite->GetNextKeyframeTime(inVideoNodeID, inIndex, inTime, outKeyframeTime, outKeyframeInterpolationMode);
		return error;
	}
	else {
		A_long outTime = 0;
		prSuiteError error = KeyFrameManager::getInstance().getNextKeyFrameTime(inIndex + 1, inTime, &outTime);
		if (error == suiteError_NoError) {
			*outKeyframeTime = outTime;
			return error;
		}
		else
			return error;
	}
}

bool KeyFrameManager::needsInterPolation(csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time) {
	if (isFirstKeyFrameTimeOrEarlier(nodeID, videoSegmentSuite, paramIndex, time)) {
		return false;
	}
	else if (isLastKeyFrameTimeOrLater(nodeID, videoSegmentSuite, paramIndex, time)) {
		return false;
	}
	else if (isExactlyOnKeyFrame(nodeID, videoSegmentSuite, paramIndex, time)) {
		return false;
	}
	else {
		return true;
	}
}

bool KeyFrameManager::isExactlyOnKeyFrame(csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time) {
	PrTime inTime = LONG_MIN;
#ifndef AE_OS_WIN // negative in time leads to incorrect results on mac API
	inTime = 0;
#endif
	PrTime outTime = 0;
	csSDK_int32 keyframeInterpolationMode;
	prSuiteError result = suiteError_NoError;

	while (result != suiteError_NoKeyframeAfterInTime) {
		result = GetNextKeyframeTime(nodeID, videoSegmentSuite, paramIndex - 1, inTime, &outTime, &keyframeInterpolationMode);

		if (outTime == time) {
			return true;
		}
		inTime = outTime;
	}
	return false;
}

bool KeyFrameManager::isFirstKeyFrameTimeOrEarlier(csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time) {

	PrTime inTime = LONG_MIN;
#ifndef AE_OS_WIN // negative in time leads to incorrect results on mac API
	inTime = 0;
#endif
	PrTime outTime = 0;
	csSDK_int32 keyframeInterpolationMode;
	prSuiteError result = suiteError_NoError;

	result = GetNextKeyframeTime(nodeID, videoSegmentSuite, paramIndex - 1, inTime, &outTime, &keyframeInterpolationMode);

	if (result == suiteError_NoKeyframeAfterInTime)
		return true;
	else if (outTime >= time)
		return true;
	else
		return false;
}

bool KeyFrameManager::isLastKeyFrameTimeOrLater(csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time) {

	PrTime inTime = time;
	PrTime outTime = 0;
	csSDK_int32 keyframeInterpolationMode;
	prSuiteError result = suiteError_NoError;

	result = GetNextKeyframeTime(nodeID, videoSegmentSuite, paramIndex - 1, inTime, &outTime, &keyframeInterpolationMode);

	return result == suiteError_NoKeyframeAfterInTime;
}

PrTime KeyFrameManager::getPreviousKeyFrameTime(csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time) {
	PrTime inTime = LONG_MIN;
#ifndef AE_OS_WIN // negative in time leads to incorrect results on mac API
	inTime = 0;
#endif
	PrTime outTime = 0;

	while (inTime < time) {
		outTime = inTime;
		inTime = getNextKeyFrameTime(nodeID, videoSegmentSuite, paramIndex, inTime);
	}

	return outTime;
}

PrTime KeyFrameManager::getNextKeyFrameTime(csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time) {
	PrTime inTime = time;
	PrTime outTime = 0;
	csSDK_int32 keyframeInterpolationMode;
	prSuiteError result = suiteError_NoError;

	result = GetNextKeyframeTime(nodeID, videoSegmentSuite, paramIndex - 1, inTime, &outTime, &keyframeInterpolationMode);

	if (result == suiteError_NoKeyframeAfterInTime) {
		//this should be impossible!
		throw "this should be impossible! (handled by isLast... isFirst... methods";
	}

	return outTime;
}

float KeyFrameManager::getRelativeKeyFrameAlpha(PF_ParamDef* params[], csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime currentTime, PrTime timestep, double offset) {
	if (KeyFrameManager::getInstance().isAE)
		currentTime = KeyFrameManager::getInstance().getCurrentAETime();

	PrTime offsetTime = (PrTime)(currentTime + timestep * offset);

	PrTime prevTime = getPreviousKeyFrameTime(nodeID, videoSegmentSuite, paramIndex, currentTime);
	PrTime nextTime = getNextKeyFrameTime(nodeID, videoSegmentSuite, paramIndex, currentTime);
	PrTime prevTimeOffset = getPreviousKeyFrameTime(nodeID, videoSegmentSuite, paramIndex, offsetTime);
	PrTime nextTimeOffset = getNextKeyFrameTime(nodeID, videoSegmentSuite, paramIndex, offsetTime);

	if (prevTime == prevTimeOffset && nextTime == nextTimeOffset) {
		currentTime = offsetTime;
	}

	double totalDiff = nextTime - prevTime;
	double prevDiff = currentTime - prevTime;

	double alpha = prevDiff / totalDiff;

	return (float)alpha;
}

int KeyFrameManager::getPreviousCamera_Pr_CPU(PF_InData* in_data, PrTime &prevDiff) {
	PrTime current = in_data->current_time;
	PrTime prevCamTime = findNeighbourKeyframeTime_CPU_Premiere(AUX_CAM_SEQUENCE, in_data, true);

	prevDiff = abs(prevCamTime - current);

	PF_ParamDef	def;
	AEFX_CLR_STRUCT(def);

	PF_CHECKOUT_PARAM(in_data, AUX_CAM_SEQUENCE, prevCamTime, in_data->time_step, in_data->time_scale, &def);
	float cam = (int)def.u.fs_d.value;
	PF_CHECKIN_PARAM(in_data, &def);
	return (int)round(cam);
}

int KeyFrameManager::getNextCamera_Pr_CPU(PF_InData* in_data, PrTime &nextDiff) {
	PrTime current = in_data->current_time;
	PrTime prevCamTime = findNeighbourKeyframeTime_CPU_Premiere(AUX_CAM_SEQUENCE, in_data, false);

	nextDiff = abs(prevCamTime - current);

	PF_ParamDef	def;
	AEFX_CLR_STRUCT(def);

	PF_CHECKOUT_PARAM(in_data, AUX_CAM_SEQUENCE, prevCamTime, in_data->time_step, in_data->time_scale, &def);
	float cam = def.u.fs_d.value;
	PF_CHECKIN_PARAM(in_data, &def);
	return (int)round(cam);
}

int KeyFrameManager::getPreviousCamera(PF_ParamDef* params[], csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time) {

	int outValue = 0;

	if (!KeyFrameManager::getInstance().isAE)
	{
		PrTime queryTime = time;

		if (needsInterPolation(nodeID, videoSegmentSuite, paramIndex, time)) {
			queryTime = getPreviousKeyFrameTime(nodeID, videoSegmentSuite, paramIndex, time);
		}
		outValue = (int)round(GetParam(nodeID, videoSegmentSuite, paramIndex, queryTime).mFloat64);
	}
	else {
		PrTime queryTime = KeyFrameManager::getInstance().getCurrentAETime();
		PrTime aeTime = queryTime;

		if (needsInterPolation(nodeID, videoSegmentSuite, paramIndex, aeTime)) {
			queryTime = getPreviousKeyFrameTime(nodeID, videoSegmentSuite, paramIndex, aeTime);
		}
		outValue = KeyFrameManager::getInstance().getKeyFrameValue(paramIndex, queryTime);
	}

	if (outValue == 0) {
		if(!KeyFrameManager::getInstance().isAE)
			outValue = (int)round(GetParam(nodeID, videoSegmentSuite, paramIndex, time).mFloat64);
		else if(params)
			outValue = (int)round(params[paramIndex]->u.fs_d.value);
		//TEMP
		if (outValue == 0)
			outValue = 1;
	}

	return  outValue;

}

int KeyFrameManager::getNextCamera(PF_ParamDef* params[], csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time) {

	int outValue = 0;

	if (!KeyFrameManager::getInstance().isAE)
	{
		PrTime queryTime = time;

		if (needsInterPolation(nodeID, videoSegmentSuite, paramIndex, time)) {
			queryTime = getNextKeyFrameTime(nodeID, videoSegmentSuite, paramIndex, time);
		}
		outValue = (int)round(GetParam(nodeID, videoSegmentSuite, paramIndex, queryTime).mFloat64);
	}
	else {
		PrTime queryTime = KeyFrameManager::getInstance().getCurrentAETime();
		PrTime aeTime = queryTime;

		if (needsInterPolation(nodeID, videoSegmentSuite, paramIndex, aeTime)) {
			queryTime = getNextKeyFrameTime(nodeID, videoSegmentSuite, paramIndex, aeTime);
		}
		outValue = KeyFrameManager::getInstance().getKeyFrameValue(paramIndex, queryTime);
	}

	if (outValue == 0) {
		if (!KeyFrameManager::getInstance().isAE)
			outValue = (int)round(GetParam(nodeID, videoSegmentSuite, paramIndex, time).mFloat64);
		else if(params)
			outValue = (int)round(params[paramIndex]->u.fs_d.value);
		//TEMP
		if (outValue == 0)
			outValue = 1;
	}

	return  outValue;
}
