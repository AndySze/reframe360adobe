#pragma once

#include <map>
#include "AE_Effect.h"
#include "PrSDKMALErrors.h"

typedef std::map<A_long, float> KeyFrameMap;

class KeyFrameData
{
public:
	KeyFrameData();
	~KeyFrameData();

	prSuiteError getNextKeyFrameTime(int param, A_long inTime, A_long *outTime);
	void addKeyFrame(int param, A_long time, float value);
	float getKeyFrameValue(int param, A_long time);
	void removeKeyFrame(int param, A_long time);
	void reset();

private:
	std::map<int, KeyFrameMap> _paramKeyFrames = std::map<int, KeyFrameMap>();
};

