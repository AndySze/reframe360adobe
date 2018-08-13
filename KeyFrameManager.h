#pragma once

#include <map>
#include <set>
#include "Reframe360.h"
#include "KeyFrameData.h"
#include "PrSDKMALErrors.h"

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

	std::set<int> _uniqueIDs = std::set<int>();

	A_long _currentAETime;

public:
	KeyFrameManager(KeyFrameManager const&) = delete;
	void operator=(KeyFrameManager const&) = delete;

	prSuiteError getNextKeyFrameTime(int param, A_long inTime, A_long *outTime);

	void addKeyFrame(int param, A_long time, int value);

	float getKeyFrameValue(int param, A_long time);

	void beginKeyNewKeyframeData();

	void setCurrentAETime(A_long time);
	A_long getCurrentAETime();

	bool isAE = false;

	//int getNewUniqueEffectID();
};

