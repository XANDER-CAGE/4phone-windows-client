/*
 * 4phone provisioning support for MicroSIP.
 *
 * This file is distributed under the GNU General Public License,
 * version 2 or any later version.
 */

#pragma once

#include "FourPhoneApi.h"
#include "global.h"

enum FourPhoneRestoreResult
{
	FourPhoneRestoreSuccess,
	FourPhoneRestoreOfflineFallback,
	FourPhoneRestoreLoginRequired
};

class CFourPhoneProvisioning
{
public:
	static FourPhoneRestoreResult TryRestoreSession(
		FourPhoneSipConfig& config,
		CString& extensionId,
		CString& error);
	static bool ShowLogin(
		CWnd* parent,
		FourPhoneSipConfig& config,
		CString& extensionId);
	static Account BuildAccount(const FourPhoneSipConfig& config);
	static bool SaveAccount(const Account& account, CString& error);
	static bool Logout(CString& error);

private:
	static CString LoadSelectedExtensionId();
	static void SaveSelectedExtensionId(const CString& extensionId);
	static bool IsLoggedOut();
	static bool SetLoggedOut(bool loggedOut);
};
