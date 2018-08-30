#pragma once
#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "AEConfig.h"

#include "PrSDKTypes.h"
#include "AE_Effect.h"
#include "A.h"
#include "AE_Macros.h"
#include "AEFX_SuiteHandlerTemplate.h"

#include "GumroadLicenseHandler.h"
#include "AEGP_SuiteHandler.h"

// This is a helper class for the aescripts licensing framework,
// providing some more high-level functionality specific to After Effects and Premiere Pro as host application
// version 3.4.1 March 2018

#ifndef TMSGDLG_H
#include "TMsgDlg.h"
#endif

#include <string>
#include <sstream>
#include <cstdlib>
using namespace std;

namespace grlic
{

	static LicenseData licenseData;
	static int lastOveruseState = -1;
	static bool lc_init = false;

#if defined(AE_OS_MAC) || defined(AE_OS_WIN)

	int getLicense(LicenseData& lData, AEGP_SuiteHandler* suitesP, bool renderOnlyMode = false, const char* tokenName = "Layout", A_long tokenValue = 1);
	PF_Err showRegistrationDialog(AEGP_SuiteHandler* suitesP, LicenseData& lData, bool useScripting = true);

	static std::string intToString(int i)
	{
		std::stringstream ss;
		std::string s;
		ss << i;
		s = ss.str();
		return s;
	}

	static string strReplace(const string s, const string search, string replace)
	{
		string str = s;
		size_t pos = 0;
		while ((pos = str.find(search, pos)) != string::npos) {
			str.replace(pos, search.size(), replace);
			pos += replace.size();
		}
		return str;
	}

	inline void openOS(const char* cmd)
	{
#ifdef AE_OS_WIN
		ShellExecute(NULL, "open", cmd, NULL, NULL, SW_SHOWNORMAL);
#else
		string s = "open " + string(cmd);
		system(s.c_str());
#endif
	}

	int getLicense(LicenseData& lData, AEGP_SuiteHandler* suitesP, bool renderOnlyMode, const char* tokenName, A_long tokenValue)
	{
		A_Err err = A_Err_NONE;
		AEGP_PersistentBlobH blobH = NULL;
		A_Boolean foundB = FALSE;
		if (suitesP) {
			ERR(suitesP->PersistentDataSuite3()->AEGP_GetApplicationBlob(&blobH));
			ERR(suitesP->PersistentDataSuite3()->AEGP_DoesKeyExist(blobH, LIC_FILENAME, tokenName, &foundB));
		}
		if (foundB) {
			return loadLicenseFromFile(LIC_FILENAME, LIC_PRODUCT_ID, LIC_PRIVATE_NUM, lData, false, renderOnlyMode);
		}
		else {
			return -1;
		}
	}

	static std::string getLicenseDataAsString(const LicenseData lData)
	{
		string lic = string(lData.licType);
		if (lic == "FLT") {
			return "Floating License";
		}
		if (lData.nofUsers == 0) return "DEMO VERSION";
		string n = intToString(lData.nofUsers);
		string regName = string(lData.firstName) + " " + string(lData.lastName);
		string myLicense;
		string multiLicense = (n == "1") ? " for 1 User" : " for " + n + " Users";
		if (lic == "SUL") myLicense = "- License" + multiLicense;
		else if (lic == "BTA") myLicense = "- Beta License";
		else if (lic == "EDU") myLicense = "- Education License";
		else if (lic == "REN") myLicense = "- Render-Only License";
		else myLicense = "- Invalid License";
		return regName + " " + myLicense;
	}

	static std::string errorCodeToMessage(const int error)
	{
		if (error == 0) return "OK";
		switch (error) {
		case -1: return "Invalid license (-1)";
		case -2: return "Invalid license (-2)";
		case -3: return "License file not found (-3)";
		case -4: return "License file corrupted (-4)";
		case -5: return "Generic error (-5)";
		case -6: return "Invalid product name (-6)";
		case -7: return "Trial Expired (-7)";
		case -8: return "Invalid license (-8) - Please double check that the license code was entered correctly";
		case -9: return "Cannot connect to server (-9) - Please make sure the license server is running properly";
		case -10: return "No free slots (-10) - There are no more free slots on the license server";
		case -11: return "Unknown license (-11) - The license cannot be found on the license server";
		case -12: return "Unknown license (-12) - The license you are trying to deactivate is not found on the license server";
		case -13: return "Client blacklisted (-13) - Your client IP is blacklisted on the license server";
		case -14: return "Network adapter error (-14) - Error reading network adapter data";
		default: return "Unknown Error";
		}
	}

	inline int checkLicense(AEGP_SuiteHandler* suitesP, LicenseData& lData, licString& errorString)
	{
		string errorMsg = "";
		int licResult = getLicense(lData, suitesP, false);
		lData.registered = (licResult == 0);
		lData.renderOK = true;

		if (strcmp(lData.licType, "BTA") == 0) {
			// only check this if we are NOT in beta mode
#ifndef LIC_BETA
			// PF_SPRINTF(out_data->return_msg, "The plugin '%s' uses a beta-only license that is no longer valid in this version!", LIC_PRODUCT_NAME);
			errorMsg = "The plugin '" + string(LIC_PRODUCT_NAME) + "' uses a beta-only license that is no longer valid in this version!";
			lData.renderOK = false; // set flag to disable rendering
#endif
		}
		strcpy(errorString, errorMsg.substr(0, 127).c_str());
		return licResult;
	}

	PF_Err showRegistrationDialog(AEGP_SuiteHandler* suitesP, LicenseData& lData, bool useScripting)
	{
		lastOveruseState = -1;

		PF_Err err = PF_Err_NONE;
		unsigned int i = 0;
		string result;
		licString errorString;
		string reg_text;
		string reg_msg;
		string sUrlRetrieve = "https://aescripts.com/downloadable/customer/products/";
		string sUrlOrder = "https://aescripts.com/";
		string sUrlExchange = "https://license.aescripts.com/exchange";
		string fxn;
		string fxno = string(LIC_PRODUCT_NAME);
		verString ver;
#ifdef pluginVersion
#if defined(WIN32)
		strcpy_s(ver, 10, pluginVersion);
#else
		strcpy(ver, pluginVersion);
#endif
#else
		ver[0] = '1';
		ver[1] = '\0';
#endif
		for (unsigned int ii = 0; ii < fxno.length(); ii++) if (fxno[ii] >= 32) fxn += fxno[ii]; else break;

		
		int reg_result = 0;
#ifdef TMSGDLG_H
		checkLicense(suitesP, lData, errorString);

		reg_text = getLicenseDataAsString(lData);
		CTMsgDlg dlg(450, 180);
		int dok = dlg.ShowDialog(
			fxn + " Registration",
			"Registered to: " + reg_text,
			"Purchase", "Cancel", lData.nofUsers %2 == 0 ? "Activate" : "Deactivate", true, "");
		result = dlg.sInputText;
		if (dok == 0) result = "";
		else if (dok == 1 && dlg.lBtnSelected == -1) {
			if (result != "" && lData.nofUsers <= 0) dlg.lBtnSelected = 3;
			if (result == "-" && lData.nofUsers > 0) dlg.lBtnSelected = 3;
		}

		if (dlg.lBtnSelected == 1) { // purchase
			openOS(sUrlOrder.c_str());
			return err;
		}
		else if (dlg.lBtnSelected == 3) {
			if (lData.nofUsers %2 != 0) { // deactivate
				licString l = {};
				l[0] = 't'; l[1] = 125; l[2] = '\0';
				saveLicenseToFile(LIC_FILENAME, LIC_PRODUCT_ID, l, ver);
				checkLicense(suitesP, lData, errorString);
				lData.registered = false;
				lData.nofUsers ++;
				reg_result = 2;
			}
			else if (result.length() > 0) { // activate
				if (result.length() > 6) {
					licString l;
					strcpy(l, result.c_str());

					int valid = 0;
					if (result[0] != '@') valid = validateLicense(l, LIC_PRODUCT_ID, LIC_PRIVATE_NUM, lData, false);
					if (valid < 0 && lData.nofUsers > 0) {
						reg_result = -1;
						reg_msg = errorCodeToMessage(-1);
					}
					else {
						saveLicenseToFile(LIC_FILENAME, LIC_PRODUCT_ID, l, ver);
						int lresult = checkLicense(suitesP, lData, errorString);
						if (lData.registered)
							reg_result = 1;
						else {

							reg_result = -1;
							reg_msg = errorCodeToMessage(-1);
						}
					}
				}
			}
		}
		else { // cancel
			return err;
		}
		string sMsg;
		if (reg_result == -1) {
			sMsg = reg_msg;
		}
		else if (reg_result == 1) {
			sMsg = "Registration successful";
		}
		else if (reg_result == 2) {
			sMsg = "License was deactivated";
		}
		CTMsgDlg dlg2(400, 150);
		dlg2.ShowDialog(fxn + " Registration", sMsg, "", "", "OK", false, "");

#endif
		checkLicense(suitesP, lData, errorString);
		return err;
	}

	inline int checkLicenseAE(PF_Cmd cmd, PF_InData *in_data, PF_OutData *out_data, LicenseData& lData)
	{
		if ((cmd == PF_Cmd_SEQUENCE_SETUP || cmd == PF_Cmd_SEQUENCE_RESETUP)) {
			if (!lc_init) {
				// check for license when a new instance of the plugin is created (SequenceSetup) or an old one is loaded
				lc_init = true;
				AEGP_SuiteHandler suites(in_data->pica_basicP);
				licString errorString;
				bool doCheck = true;
				/*if (cmd == PF_Cmd_SEQUENCE_RESETUP) {
				if (lData.registered) doCheck = false;
				}*/
				if (doCheck) {
					checkLicense(&suites, lData, errorString);
					PF_SPRINTF(out_data->return_msg, errorString);
				}
			}
			checkOveruse(lData);
		}
		else if (cmd == PF_Cmd_USER_CHANGED_PARAM || cmd == PF_Cmd_UPDATE_PARAMS_UI) {
			checkOveruse(lData);
		}
		else if (cmd == PF_Cmd_RENDER || cmd == PF_Cmd_SMART_RENDER) {
			// check for license overuse on render
			if (lData.renderOK == false) return -1;
			if (lData.nofUsers > 0) { // only if we have a licensed user
									  // lData.registered = (lData.overused<=0);
				if (lData.overused != lastOveruseState) {
					if (lData.overused == 1) { // currently overused?
						PF_SPRINTF(out_data->return_msg, "'%s' is already being used by the maximum number of licenses allowed. You need to release a license from another machine to license this one. Until then, this instance will run in trial mode.",
							LIC_PRODUCT_NAME);
					}
					else if (lData.overused == 0 && lastOveruseState == 1) { // changing from overused to ok
						PF_SPRINTF(out_data->return_msg, "'%s' has been successfully licensed.", LIC_PRODUCT_NAME);
					}
					lastOveruseState = lData.overused;
				}
			}
		}
		return 0;
	}

#endif // AE_OS_XXX

} // namespace aescripts
