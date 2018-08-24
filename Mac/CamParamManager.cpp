#include "CamParamManager.h"

ParamSet CamParamManager::getParams(int effect_id) {
	return _paramSets[effect_id];
}

void CamParamManager::setParams(ParamSet paramSet) {
	_paramSets[paramSet.id] = paramSet;
}

void CamParamManager::setCurrentID(int nodeId) {
	_id = nodeId;
}

int CamParamManager::getCurrentID() {
	return _id;
}

void CamParamManager::initParams(int nodeId) {
	ParamSet paramSet;
	paramSet.id = nodeId;
	_paramSets[nodeId] = paramSet;
}