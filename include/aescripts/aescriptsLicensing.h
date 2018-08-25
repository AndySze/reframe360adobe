#pragma once

#define AE_LICLIB_VERSION "TLC34"

/*
TLC34: version 3.4 February 2018
- bugfix for overuse checks
- full support for VS2010 (overuse functionality was disabled before for that compiler)

TLC33: version 3.3 January 2018
- bugfix for render-only or non single-user licenses

TLC32: version 3.2 November 2017
- various bugfixes for "overuse" checks
- network adapter detection improved
- bugfix for trial days detection
- improved error handling

TLC31: version 3.1 September 2017
- registration dialog now reports if a license does not exist on the server
- userID/logged user name bugfix on MacOS

TLC30: version 3.0 July 2017
- preserve license string in lData
- overuse support
- registered flag now part of lData
- string trimming/matching in serial input dialog
- complete rewrite of many functions
- the license server now aggregates licenses for a single product under the laster serial entered in the list, not the first one
- the aescriptsLicTool now supports error code -9 (no connection to the remote server)

TLC22: version 2.2 Feb 5, 2016
- improved license test AE plugin
- memory safeguards on initialisation
- support for libstd++ on MacOS
- support for trial licenses (expire after x days)

TLC21: version 2.1 Oct 30, 2015
- Visual Studio 2015 compatibility
- more flexible license file loading in restricted environments
- improved license test AE plugin
- MacOS: preferences folder is now "/Library/Application Support"

TLC20: version 2.0 July 21, 2014 
- dynamic blacklisting
- Premiere Pro compatible UI
- code cleanup
- Visual Studio 2013 compatibility
- license can now be deactivated in the dialog
*/


#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#ifndef AESCRIPTS_MANUAL_LINK
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#endif
#endif
#endif

#ifndef AESCRIPTSLICENSING_H
#define AESCRIPTSLICENSING_H

namespace aescripts
{

typedef char licString[128];
typedef char machineString[60];
typedef char verString[11];
typedef char licVerString[6];

struct LicenseData
{
	char firstName[30]; // user's first name
	char lastName[30]; // user's last name
	int nofUsers; // number of licensed users
	char licType[10]; // license type (SUL, BTA, EDU, REN)
	char productID[30]; // product ID
	char serial[30]; // license serial string

	licString license; // complete license string
	verString productVersion; // product version
	licVerString licenseVersion; // version of the license file

	int firstStartTimestamp; // timestamp of first start
	int numberOfDaysSinceFirstStart;
	bool registered;
	int overused;//-1: unknown, 0: no, 1: yes
	bool renderOK;
};

// load license file from disk
int loadLicenseFromFile(const char* _licenseFileName, const char* _productID, int _privNum, LicenseData& _lData, bool _createTrialLicense = true, bool _renderOnlyMode = false);

// save license to disk
int saveLicenseToFile(const char* _licenseFileName, const char* _productID, licString _lic, verString _effectVersion);

// validate license string (usually not needed, as called implicitly by loadLicenseFromFile)
int validateLicense(licString _license, const char* _productID, int _privNum, LicenseData& _lData, bool _renderOnlyMode = false);

// get machine ID (usually not needed)
bool getMachineId(machineString& _result);

// dynamically add data for a blacklisted serial
void addBlacklistedSerial(int _privNum, int _d1, int _d2, int _d3, int _d4);

// get overuse state (usually not needed to be called directly, called implicitly by validateLicense)
int checkOveruse(LicenseData& _lData);

// get server and port for (optional) licensing server
int getLicenseServerPort(licString& _cServer, licString& _cPort, licString& _cBackupServer, licString& _cBackupPort);

// get floating license from (optional) remote licensing server
int getLicenseFromLicenseServer(const char* _productID, int _privNum, bool _drop, licString& _license);

} // namespace aescripts

#endif // AESCRIPTSLICENSING_H
