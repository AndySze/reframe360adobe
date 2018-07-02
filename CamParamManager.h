#pragma once

#include "SDK_CrossDissolve.h"
#include <map>

using namespace std;

class CamParamManager
{
public:
	static CamParamManager& getInstance()
	{
		static CamParamManager    instance; // Guaranteed to be destroyed.
							  // Instantiated on first use.
		return instance;
	}
private:
	CamParamManager() {}                    // Constructor? (the {} brackets) are needed here.

	map<int, ParamSet> _paramSets;

	int _id = 0;

							  // C++ 11
							  // =======
							  // We can use the better technique of deleting the methods
							  // we don't want.
public:
	CamParamManager(CamParamManager const&) = delete;
	void operator=(CamParamManager const&) = delete;

	ParamSet getParams(int effect_id);
	void setParams(ParamSet paramSet);

	void setCurrentID(int nodeId);
	int getCurrentID();

	void initParams(int nodeId);

	// Note: Scott Meyers mentions in his Effective Modern
	//       C++ book, that deleted functions should generally
	//       be public as it results in better error messages
	//       due to the compilers behavior to check accessibility
	//       before deleted status
};

