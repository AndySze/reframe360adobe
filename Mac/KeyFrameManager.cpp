#include "KeyFrameManager.h"

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

/*int KeyFrameManager::getNewUniqueEffectID() {
	int maxEffectID = 0;

	int numIds = _uniqueIDs.size();

	set<int>::iterator it = _uniqueIDs.begin();

	for (int i = 0; i < numIds; i++) {
		int id = *next(it);
		if (id > maxEffectID) {
			maxEffectID = id;
		}
	}

	int newId = maxEffectID + 1;
	_uniqueIDs.insert(newId);

	return newId;
}*/