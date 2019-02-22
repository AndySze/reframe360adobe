#pragma once

#include <map>
#include <set>
#include "Reframe360.h"
#include "KeyFrameData.h"
#include "PrSDKMALErrors.h"
#include "PrSDKVideoSegmentSuite.h"


class KeyFrameManager
{
public:
	static KeyFrameManager& getInstance()
	{
		static KeyFrameManager    instance; // Guaranteed to be destroyed.
							  // Instantiated on first use.
		return instance;
	}
private:
	KeyFrameManager() {}

	KeyFrameData _sequenceKeyFrames = KeyFrameData();
    
    KeyFrameData _currentRecordingKeyFrames = KeyFrameData();

	std::set<int> _uniqueIDs = std::set<int>();

	A_long _currentAETime;
	int _camToCopy = 1;
    
    bool _isRecording = false;

	float _currentFps = 25.0f;

public:
	KeyFrameManager(KeyFrameManager const&) = delete;
	void operator=(KeyFrameManager const&) = delete;

	prSuiteError getNextKeyFrameTime(int param, A_long inTime, A_long *outTime);

	void addKeyFrame(int param, A_long time, int value);
    float getKeyFrameValue(int param, A_long time);
    void beginKeyNewKeyframeData();
    
    void setRecordingKeyframe(int param, A_long time, float value);
    KeyFrameData getCurrentRecordingKeyframeData();
    void beginKeyNewRemoteRecording();
    void stopRemoteRecording();
    bool isRecording();

	void setCurrentAETime(A_long time);
	A_long getCurrentAETime();

	void setCamToCopy(int cam);
	int getCamToCopy();

	bool isAE = false;
	bool isCpuProcessing = false;
	bool isRegistered = true;

	std::map<int, int> idToIndex;

	PrParam GetParam(
		csSDK_int32 nodeID,
		PrSDKVideoSegmentSuite* videoSegmentSuite,
		csSDK_int32 inIndex,
		PrTime inTime);

	prSuiteError GetNextKeyframeTime(
		csSDK_int32 inVideoNodeID,
		PrSDKVideoSegmentSuite* videoSegmentSuite,
		csSDK_int32 inIndex,
		PrTime inTime,
		PrTime* outKeyframeTime,
		csSDK_int32* outKeyframeInterpolationMode);

	bool needsInterPolation(csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time);

	bool isExactlyOnKeyFrame(csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time);

	bool isFirstKeyFrameTimeOrEarlier(csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time);

	bool isLastKeyFrameTimeOrLater(csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time);

	PrTime getPreviousKeyFrameTime(csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time);

	PrTime getNextKeyFrameTime(csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time);

	float getRelativeKeyFrameAlpha(PF_ParamDef* params[], csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime currentTime, PrTime timestep, double offset);

	int getPreviousCamera(PF_ParamDef* params[], csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time);

	int getNextCamera(PF_ParamDef* params[], csSDK_int32 nodeID, PrSDKVideoSegmentSuite* videoSegmentSuite, csSDK_int32 paramIndex, PrTime time);

	int getPreviousCamera_Pr_CPU(PF_InData* in_data, PrTime &prevDiff);

	int getNextCamera_Pr_CPU(PF_InData* in_data, PrTime &nextDiff);

	float getCurrentFps();
	void setCurrentFps(float fps);
};

