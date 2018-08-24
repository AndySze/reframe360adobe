#include "KeyFrameData.h"



KeyFrameData::KeyFrameData()
{
}


KeyFrameData::~KeyFrameData()
{
}

prSuiteError KeyFrameData::getNextKeyFrameTime(int param, A_long inTime, A_long *outTime) {

	if (_paramKeyFrames.size() == 0)
		return suiteError_NoKeyframeAfterInTime;

	std::map<A_long, KeyFrameMap>::iterator it1 = _paramKeyFrames.find(param);
	if (it1 == _paramKeyFrames.end())
		return suiteError_NoKeyframeAfterInTime;

	if (_paramKeyFrames[param].size() == 0)
		return suiteError_NoKeyframeAfterInTime;

	int keyCount = _paramKeyFrames[param].size();

	A_long maxTime = -1;

	for (std::map<A_long, float>::iterator it = _paramKeyFrames[param].begin(); it != _paramKeyFrames[param].end(); ++it) {
		A_long time = it->first;

		if (time > maxTime)
			maxTime = time;

		if (time > inTime)
			break;
	}

	if (maxTime == -1 || maxTime == inTime || maxTime < inTime)
		return suiteError_NoKeyframeAfterInTime;
	else
		*outTime = maxTime;

	return suiteError_NoError;
}

void KeyFrameData::addKeyFrame(int param, A_long time, float value) {
	if (_paramKeyFrames.count(param) == 0)
		_paramKeyFrames[param] = KeyFrameMap();
	_paramKeyFrames[param][time] = value;
}

float KeyFrameData::getKeyFrameValue(int param, A_long time) {
	if (_paramKeyFrames.count(param) != 0)
		return _paramKeyFrames[param][time];
	else
		return 0;
}

void KeyFrameData::removeKeyFrame(int param, A_long time) {
	if (_paramKeyFrames.count(param) != 0)
		_paramKeyFrames[param].erase(time);
}

void KeyFrameData::reset() {
	_paramKeyFrames.clear();
}