#include "GumroadLicenseHandler.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

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

string grlic::getLicenseStoreDir() {
#ifdef _WIN32
	TCHAR szPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, szPath)))
	{
		string dirString = string("[AppData]\\aescripts\\[pluginname]\\[pluginname].lic");
		dirString = strReplace(dirString, "[pluginname]", LIC_FILENAME);
		string appDataString = string(szPath);
		dirString = strReplace(dirString, "[AppData]", appDataString);
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
	throw exception("not implemented");
}

// save license to disk
int grlic::saveLicenseToFile(const char* _licenseFileName, const char* _productID, licString _lic, verString _effectVersion) {
	throw exception("not implemented");
}

// validate license string (usually not needed, as called implicitly by loadLicenseFromFile)
int grlic::validateLicense(licString _license, const char* _productID, int _privNum, LicenseData& _lData, bool _renderOnlyMode) {
	throw exception("not implemented");
}

// get machine ID (usually not needed)
bool grlic::getMachineId(machineString& _result) {

	string hardwareUID = string(getSystemUniqueId());

	int len = hardwareUID.copy(_result, sizeof(machineString), 0);

	return len;
}

// dynamically add data for a blacklisted serial
void grlic::addBlacklistedSerial(int _privNum, int _d1, int _d2, int _d3, int _d4) {
	throw exception("not implemented");
}

// get overuse state (usually not needed to be called directly, called implicitly by validateLicense)
int grlic::checkOveruse(LicenseData& _lData) {
	throw exception("not implemented");
}

// get server and port for (optional) licensing server
int grlic::getLicenseServerPort(licString& _cServer, licString& _cPort, licString& _cBackupServer, licString& _cBackupPort) {
	throw exception("not implemented");
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
	throw exception("not implemented");
}
#define PARSEFAIL ld.success=false;return ld;
GumroadLicData parseJson(string s) {
	GumroadLicData ld;

	std::istringstream iss(s);
	std::vector<std::string> results((std::istream_iterator<WordDelimitedByComma>(iss)),
		std::istream_iterator<WordDelimitedByComma>());

	json j = json::parse(s);
	int error = 0;

	ld.success = false;
	if (j.count("success")) {
		ld.success = j["success"];
	}
	if (!ld.success) { PARSEFAIL }

	if (j.count("uses"))
		ld.uses = j["uses"];
	else {PARSEFAIL}

	json purchase;
	if (j.count("purchase"))
		purchase = j["purchase"].get<json>();
	else {PARSEFAIL}

	if (purchase.count("id"))
		ld.id = purchase["id"].get<std::string>();
	else { PARSEFAIL }

	if (purchase.count("product_name"))
		ld.product_name = purchase["product_name"].get<std::string>();
	else { PARSEFAIL }

	if (purchase.count("created_at")) {
		string timeString = purchase["created_at"].get<std::string>();

		char dummy[10];
		sscanf(timeString.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d%s",
			&ld.created_at.tm_year,
			&ld.created_at.tm_mon,
			&ld.created_at.tm_mday,
			&ld.created_at.tm_hour,
			&ld.created_at.tm_min,
			&ld.created_at.tm_sec,
			dummy);
	}
	else { PARSEFAIL }

	if (purchase.count("full_name"))
		ld.full_name = purchase["full_name"].get<std::string>();

	if (purchase.count("variants"))
		ld.variants = purchase["variants"].get<std::string>();
	else { PARSEFAIL }

	if (purchase.count("refunded"))
		ld.refunded = purchase["refunded"].get<bool>();
	else { PARSEFAIL }

	if (purchase.count("chargebacked"))
		ld.chargebacked = purchase["chargebacked"].get<bool>();
	else { PARSEFAIL }

	if (purchase.count("email"))
		ld.email = purchase["email"].get<std::string>();
	else { PARSEFAIL }

	return ld;
}

void grlic::testLicenseCheck() {
	string uuid = getSystemUniqueId();

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

		parseJson(resp);

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