#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "GumroadLicenseHandler.h"
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <string>
#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>
#include <iomanip>

#ifdef _WIN32
#include <KnownFolders.h>
#include <ShlObj.h>
#define CSIDL_APPDATA 0x001a
#endif

#include "HardwareUUIDHandler.h"

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

using namespace grlic;
using json = nlohmann::json;

GumroadLicData queryGumroadLicensedata(string serial, bool increaseCount);

string strReplace(const string s, const string search, string replace)
{
	string str = s;
	size_t pos = 0;
	while ((pos = str.find(search, pos)) != string::npos) {
		str.replace(pos, search.size(), replace);
		pos += replace.size();
	}
	return str;
}

//creates it if it doesn't exist
string grlic::getLicenseStoreDir() {
#ifdef _WIN32
	TCHAR szPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, szPath)))
	{
		string dirString = string("[AppData]\\gumroad\\[pluginname]\\");
		dirString = strReplace(dirString, "[pluginname]", LIC_FILENAME);
		string appDataString = string(szPath);
		dirString = strReplace(dirString, "[AppData]", appDataString);

		const char* path = dirString.c_str();
		boost::filesystem::path dir(path);
		if (boost::filesystem::create_directories(dir))
		{
			std::cerr << "Directory Created: " << dirString << std::endl;
		}

		string fileString = string("[pluginname].lic");
		fileString = strReplace(fileString, "[pluginname]", LIC_FILENAME);
		dirString.append(fileString);

		return dirString;
	}
	throw exception("couldn't get appdata directory!");
#else
	string dirString = string("~/Library/Application Support/com.aescripts.[pluginname].lic");
	dirString.replace(dirString.begin(), dirString.end(), "[pluginname]", LIC_FILENAME);
	return dirString;
#endif
}

// load license file from disk
int grlic::loadLicenseFromFile(const char* _licenseFileName, const char* _productID, int _privNum, LicenseData& _lData, bool _createTrialLicense, bool _renderOnlyMode) {
	throw runtime_error("not implemented");
}

typedef enum {
	KEY_GOOD = 0,
	KEY_INVALID,
	KEY_BLACKLISTED,
	KEY_PHONY,
}KEY_VALIDITY;

template <typename I>
std::string n2hexstr(I w, size_t hex_len = sizeof(I) << 1) {
	static const char* digits = "0123456789ABCDEF";
	std::string rc(hex_len, '0');
	for (size_t i = 0, j = (hex_len - 1) * 4; i<hex_len; ++i, j -= 4)
		rc[i] = digits[(w >> j) & 0x0f];
	return rc;
}

u_char PKV_GetKeyByte(int seed, int a, int b, int c) {
	int i_a = a % 25;
	int i_b = b % 3;
	u_char i_out = 0;

	if (i_a % 2 == 0)
		i_out = ((seed >> a) & 0x000000ff) ^ ((seed >> b) | c);
	else
		i_out = ((seed >> a) & 0x000000ff) ^ ((seed >> b) & c);

	return i_out;
}

string PKV_GetChecksum(string s) {
	int16_t left = 0x0056;
	int16_t right = 0x00af;

	if (s.length() > 0) {
		for (int i = 0; i < s.length(); i++) {
			right = right + (int16_t)s[i];
			if (right > 0x00ff) {
				right -= 0x00ff;
			}
			left += right;
			if (left > 0x00ff) {
				left -= 0x00ff;
			}
		}
	}

	int16_t sum = (left << 8) + right;

	return n2hexstr<int16_t>(sum);
}

int16_t PKV_GetIntChecksum(string s) {
	int16_t left = 0x0056;
	int16_t right = 0x00af;

	if (s.length() > 0) {
		for (int i = 0; i < s.length(); i++) {
			right = right + (int16_t)s[i];
			if (right > 0x00ff) {
				right -= 0x00ff;
			}
			left += right;
			if (left > 0x00ff) {
				left -= 0x00ff;
			}
		}
	}

	int16_t sum = (left << 8) + right;

	return sum;
}

string PKV_MakeKey(int seed) {
	u_char keyBytes[4];

	keyBytes[0] = PKV_GetKeyByte(seed, 24, 3, 200);
	keyBytes[1] = PKV_GetKeyByte(seed, 10, 0, 56);
	keyBytes[2] = PKV_GetKeyByte(seed, 1, 2, 91);
	keyBytes[3] = PKV_GetKeyByte(seed, 7, 1, 100);

	string hex_str = n2hexstr<int>(seed);
	for (int i = 0; i < 4; i++) {
		int b = keyBytes[i];
		hex_str.append(n2hexstr<u_char>(b));
	}
	hex_str.append(PKV_GetChecksum(hex_str));
	int len = hex_str.length();
	return hex_str;
}

bool PKV_CheckKeyChecksum(string key) {
	std::string k = key;
	std::transform(k.begin(), k.end(), k.begin(), ::toupper);
	if (k.length() != 20)
		return false;

	string checksum = k.substr(16, 4);
	k = k.substr(0, 16);

	return PKV_GetChecksum(k).compare(checksum) == 0;
}

KEY_VALIDITY PKV_CheckKey(string k) {
	KEY_VALIDITY result = KEY_INVALID;
	if (!PKV_CheckKeyChecksum(k))
		return result;

	std::transform(k.begin(), k.end(), k.begin(), ::toupper);

	result = KEY_PHONY;

	string seed = k.substr(0, 8);
	int seed_int;
	try {
		std::stringstream ss;
		ss << std::hex << seed;
		ss >> seed_int;
	}
	catch (std::exception const& e) {
		return result;
	}

	string kb = k.substr(8, 2);
	u_char byte = PKV_GetKeyByte(seed_int, 24, 3, 200);
	string byte_str = n2hexstr<u_char>(byte);
	if (kb.compare(byte_str))
		return result;

	kb = k.substr(10, 2);
	byte = PKV_GetKeyByte(seed_int, 10, 0, 56);
	byte_str = n2hexstr<u_char>(byte);
	if (kb.compare(byte_str))
		return result;

	kb = k.substr(12, 2);
	byte = PKV_GetKeyByte(seed_int, 1, 2, 91);
	byte_str = n2hexstr<u_char>(byte);
	if (kb.compare(byte_str))
		return result;

	kb = k.substr(14, 2);
	byte = PKV_GetKeyByte(seed_int, 7, 1, 100);
	byte_str = n2hexstr<u_char>(byte);
	if (kb.compare(byte_str))
		return result;

	return KEY_GOOD;
}

string genLicString(string serial, string email, string hardware_uid, string pkvKey) {
	std::stringstream ss;
	ss << serial << "#" << email << "#" << hardware_uid << "#" << pkvKey;

	string checksum = PKV_GetChecksum(ss.str());

	ss << "#" << checksum;

	return ss.str();
}

int getSeed(string serial, string hwuid) {
	int16_t seedleft = PKV_GetIntChecksum(serial);
	int16_t seedright = PKV_GetIntChecksum(hwuid);
	int seed = seedleft << 16;
	seed += seedright;
	return seed;
}

string getLicString(string serial, string email) {
	string hwuid = getSystemUniqueId();
	int seed = getSeed(serial, hwuid);

	string pkvKey = PKV_MakeKey(seed);

	string licString = genLicString(serial, email, hwuid, pkvKey);
	return licString;
}

bool checkLicStringCheckSum(string licString) {
	try {
		bool result = false;

		int splitloc = licString.find_last_of("#");
		string checksum = licString.substr(splitloc + 1, 4);
		string main = licString.substr(0, splitloc);

		string calced_checksum = PKV_GetChecksum(main);

		return checksum.compare(calced_checksum) == 0;
	}
	catch (std::exception e) {
		return false;
	}
}

string getNthComponent(string str, int n) {
	std::istringstream sstr(str);
	string result;
	for(int i=0;i<=n;i++)
		getline(sstr, result, '#');
	return result;
}

string getPKVStringFromLicString(string licString) {
	return getNthComponent(licString, 3);
}

string getEmailStringFromLicString(string licString) {
	return getNthComponent(licString, 1);
}

string getHardwareUIDStringFromLicString(string licString) {
	return getNthComponent(licString, 2);
}

string getSerialStringFromLicString(string licString) {
	return getNthComponent(licString, 0);
}

bool checkLicStringValid(string licString) {
	if (!checkLicStringCheckSum(licString))
		return false;

	string lic_pkv = getPKVStringFromLicString(licString);
	string lic_serial = getSerialStringFromLicString(licString);
	string lic_hardwareUID = getHardwareUIDStringFromLicString(licString);
	string lic_email = getEmailStringFromLicString(licString);

	string hardwareUID = getSystemUniqueId();
	if (lic_hardwareUID.compare(hardwareUID))
		return false;

	int seed = getSeed(lic_serial, lic_hardwareUID);

	string pkv = PKV_MakeKey(seed);
	if (lic_pkv.compare(pkv))
		return false;

	return PKV_CheckKey(pkv) == KEY_GOOD;
}

// save license to disk
int grlic::saveLicenseToFile(const char* _licenseFileName, const char* _productID, licString _lic, verString _effectVersion) {
	// compute license hash from licString, hardware uid and effect version and save to disc

	//1.: compute seed

	//2.: 

	throw runtime_error("not implemented");
}

// validate license string (usually not needed, as called implicitly by loadLicenseFromFile)
int grlic::validateLicense(licString _license, const char* _productID, int _privNum, LicenseData& _lData, bool _renderOnlyMode) {
	throw runtime_error("not implemented");
}

// get machine ID (usually not needed)
bool grlic::getMachineId(machineString& _result) {

	string hardwareUID = string(getSystemUniqueId());

	int len = hardwareUID.copy(_result, sizeof(machineString), 0);

	return len;
}

// dynamically add data for a blacklisted serial
void grlic::addBlacklistedSerial(int _privNum, int _d1, int _d2, int _d3, int _d4) {
	throw runtime_error("not implemented");
}

// get overuse state (usually not needed to be called directly, called implicitly by validateLicense)
int grlic::checkOveruse(LicenseData& _lData) {
	throw runtime_error("not implemented");
}

// get server and port for (optional) licensing server
int grlic::getLicenseServerPort(licString& _cServer, licString& _cPort, licString& _cBackupServer, licString& _cBackupPort) {
	throw runtime_error("not implemented");
}

class WordDelimitedByComma : public std::string
{

};

std::istream& operator>>(std::istream& is, WordDelimitedByComma& output)
{
	std::getline(is, output, ',');
	return is;
}

// get floating license from (optional) remote licensing server
int grlic::getLicenseFromLicenseServer(const char* _productID, int _privNum, bool _drop, licString& _license) {
	throw runtime_error("not implemented");
}
#define PARSEFAIL ld->success=false; return;
void parseJson(GumroadLicData* ld, string s) {

	std::istringstream iss(s);
	std::vector<std::string> results((std::istream_iterator<WordDelimitedByComma>(iss)),
		std::istream_iterator<WordDelimitedByComma>());

	json j = json::parse(s);
	int error = 0;

	ld->success = false;
	if (j.count("success")) {
		ld->success = j["success"];
	}
	if (!ld->success) { PARSEFAIL }

	if (j.count("uses"))
		ld->uses = j["uses"];
	else {PARSEFAIL}

	json purchase;
	if (j.count("purchase"))
		purchase = j["purchase"].get<json>();
	else {PARSEFAIL}

	if (purchase.count("id"))
		ld->id = purchase["id"].get<std::string>();
	else { PARSEFAIL }

	if (purchase.count("product_name"))
		ld->product_name = purchase["product_name"].get<std::string>();
	else { PARSEFAIL }

	if (purchase.count("created_at")) {
		string timeString = purchase["created_at"].get<std::string>();

		char dummy[10];
		sscanf(timeString.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d%s",
			&ld->created_at.tm_year,
			&ld->created_at.tm_mon,
			&ld->created_at.tm_mday,
			&ld->created_at.tm_hour,
			&ld->created_at.tm_min,
			&ld->created_at.tm_sec,
			dummy);
	}
	else { PARSEFAIL }

	if (purchase.count("full_name"))
		ld->full_name = purchase["full_name"].get<std::string>();

	if (purchase.count("variants"))
		ld->variants = purchase["variants"].get<std::string>();
	else { PARSEFAIL }

	if (purchase.count("refunded"))
		ld->refunded = purchase["refunded"].get<bool>();
	else { PARSEFAIL }

	if (purchase.count("chargebacked"))
		ld->chargebacked = purchase["chargebacked"].get<bool>();
	else { PARSEFAIL }

	if (purchase.count("email"))
		ld->email = purchase["email"].get<std::string>();
	else { PARSEFAIL }
}

bool queryGumroadLicenseData(GumroadLicData* gld, string serial, bool increaseCount) {
	try
	{
		auto const host = "api.gumroad.com";
		auto const port = "80";
		string target = "/v2/licenses/verify?product_permalink=PkfnZ&license_key=";
		target.append(serial);
		if (!increaseCount)
			target.append("&increment_uses_count=false");


		// The io_context is required for all I/O
		boost::asio::io_context ioc;

		// These objects perform our I/O
		tcp::resolver resolver{ ioc };
		tcp::socket socket{ ioc };

		// Look up the domain name
		auto const results = resolver.resolve(host, port);

		// Make the connection on the IP address we get from a lookup
		boost::asio::connect(socket, results.begin(), results.end());

		// Set up an HTTP GET request message
		http::request<http::string_body> req{ http::verb::post, target, 10 };
		req.set(http::field::host, host);
		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		// Send the HTTP request to the remote host
		http::write(socket, req);

		// This buffer is used for reading and must be persisted
		boost::beast::flat_buffer buffer;

		// Declare a container to hold the response
		http::response<http::string_body> res;

		// Receive the HTTP response
		http::read(socket, buffer, res);

		string resp = res.body();

		parseJson(gld, resp);

		boost::system::error_code ec;
		socket.shutdown(tcp::socket::shutdown_both, ec);

		// not_connected happens sometimes
		// so don't bother reporting it.
		//
		if (ec && ec != boost::system::errc::not_connected)
			throw boost::system::system_error{ ec };

		return true;
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return false;
	}
}

void grlic::testLicenseCheck() {
	string licString = getLicString("0C630A39-D5EA4009-A43A2F35-23C69DA5", "gumroad@visuality.at");
	checkLicStringCheckSum(licString);
	checkLicStringValid(licString);



	try
	{
		auto const host = "api.gumroad.com";
		auto const port = "80";
		auto const target = "/v2/licenses/verify?product_permalink=PkfnZ&license_key=0C630A39-D5EA4009-A43A2F35-23C69DA5";

		string target_str = string(target);

		// The io_context is required for all I/O
		boost::asio::io_context ioc;

		// These objects perform our I/O
		tcp::resolver resolver{ ioc };
		tcp::socket socket{ ioc };

		// Look up the domain name
		auto const results = resolver.resolve(host, port);

		// Make the connection on the IP address we get from a lookup
		boost::asio::connect(socket, results.begin(), results.end());

		// Set up an HTTP GET request message
		http::request<http::string_body> req{ http::verb::post, target, 10 };
		req.set(http::field::host, host);
		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		// Send the HTTP request to the remote host
		http::write(socket, req);

		// This buffer is used for reading and must be persisted
		boost::beast::flat_buffer buffer;

		// Declare a container to hold the response
		http::response<http::string_body> res;

		// Receive the HTTP response
		http::read(socket, buffer, res);

		string resp = res.body();

		GumroadLicData gld;
		parseJson(&gld, resp);

		// Write the message to standard out
		std::cout << res << std::endl;

		// Gracefully close the socket
		boost::system::error_code ec;
		socket.shutdown(tcp::socket::shutdown_both, ec);

		// not_connected happens sometimes
		// so don't bother reporting it.
		//
		if (ec && ec != boost::system::errc::not_connected)
			throw boost::system::system_error{ ec };

		// If we get here then the connection is closed gracefully
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}
}
