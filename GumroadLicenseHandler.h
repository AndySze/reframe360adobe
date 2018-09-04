#pragma once

#include <string>

// this is the name of your plugin
#define LIC_PRODUCT_NAME "Reframe360"
// this is the private number - it is unique to every plugin/product and issued by aescripts
#define LIC_PRIVATE_NUM 123456
// the product ID is a string also issued by aescripts that identifies your plugin
#define LIC_PRODUCT_ID "VSLRF"
// this is the filename (extension .lic gets added automatically) at which the license is stored on the disk
// you must share the license filename with the aescripts admin so that it is entered in the main database
#define LIC_FILENAME "Reframe360"
// set this define if your plugin is compiled for beta-testers, it will then accept BTA type licenses
#define LIC_BETA

using namespace std;

typedef struct {
	bool success;
	int uses;
	string id;
	string product_name;
	tm created_at;
	string full_name;
	string variants;
	bool refunded;
	bool chargebacked;
	string email;
}GumroadLicData;

namespace grlic
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

	string getLicenseStoreDir();
	void testLicenseCheck();


} // namespace aescripts

