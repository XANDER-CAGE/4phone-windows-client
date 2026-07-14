/*
 * 4phone provisioning support for MicroSIP.
 *
 * This file is distributed under the GNU General Public License,
 * version 2 or any later version.
 */

#include "stdafx.h"
#include "FourPhoneProvisioning.h"

#include "FourPhoneCredentialStore.h"
#include "FourPhoneLoginDlg.h"
#include "langpack.h"
#include "settings.h"

namespace
{
	const TCHAR* kProvisioningSection = _T("4phone");
	const TCHAR* kEmailKey = _T("email");
	const TCHAR* kExtensionIdKey = _T("extensionId");
	const TCHAR* kLoggedOutKey = _T("loggedOut");

	void SecureClear(CString& value)
	{
		if (value.IsEmpty()) {
			return;
		}
		const int length = value.GetLength();
		LPTSTR buffer = value.GetBuffer();
		SecureZeroMemory(buffer, length * sizeof(TCHAR));
		value.ReleaseBuffer(0);
	}

	FourPhoneRestoreResult RequireLogin()
	{
		accountSettings.AccountDelete(1);
		accountSettings.accountId = 0;
		accountSettings.account = Account();
		accountSettings.account.rememberPassword = false;
		accountSettings.SettingsSave();
		WritePrivateProfileString(
			kProvisioningSection,
			kLoggedOutKey,
			_T("1"),
			accountSettings.iniFile);
		return FourPhoneRestoreLoginRequired;
	}

	FourPhoneRestoreResult ApiFailureResult(
		const CFourPhoneApi& api)
	{
		const DWORD statusCode = api.GetLastStatusCode();
		if (statusCode == 0 || statusCode >= 500) {
			return FourPhoneRestoreOfflineFallback;
		}
		return RequireLogin();
	}
}

FourPhoneRestoreResult CFourPhoneProvisioning::TryRestoreSession(
	FourPhoneSipConfig& config,
	CString& extensionId,
	CString& error)
{
	config = FourPhoneSipConfig();
	extensionId.Empty();
	error.Empty();
	if (IsLoggedOut()) {
		error = Translate(_T("Your 4phone session has ended"));
		return RequireLogin();
	}

	CString refreshToken;
	CString credentialError;
	if (!CFourPhoneCredentialStore::LoadRefreshToken(
		refreshToken,
		credentialError)) {
		error = credentialError;
		return RequireLogin();
	}

	CFourPhoneApi api;
	FourPhoneAuthTokens tokens;
	const bool refreshed = api.Refresh(refreshToken, tokens);
	SecureClear(refreshToken);
	if (!refreshed) {
		error = api.GetLastError();
		if (api.GetLastStatusCode() == 401) {
			CString deleteError;
			CFourPhoneCredentialStore::DeleteRefreshToken(deleteError);
		}
		return ApiFailureResult(api);
	}

	CString storeError;
	if (!CFourPhoneCredentialStore::SaveRefreshToken(
		tokens.refreshToken,
		storeError)) {
		error = storeError;
		SecureClear(tokens.accessToken);
		SecureClear(tokens.refreshToken);
		return RequireLogin();
	}

	api.SetAccessToken(tokens.accessToken);
	SecureClear(tokens.accessToken);
	SecureClear(tokens.refreshToken);
	std::vector<FourPhoneExtension> extensions;
	if (!api.GetExtensions(extensions)) {
		error = api.GetLastError();
		return ApiFailureResult(api);
	}
	if (extensions.empty()) {
		error = Translate(_T(
			"No active extension is assigned to your user"));
		return RequireLogin();
	}

	const CString savedExtensionId = LoadSelectedExtensionId();
	for (size_t index = 0; index < extensions.size(); index++) {
		if (extensions[index].id == savedExtensionId) {
			extensionId = extensions[index].id;
			break;
		}
	}
	if (extensionId.IsEmpty() && extensions.size() == 1) {
		extensionId = extensions[0].id;
	}
	if (extensionId.IsEmpty()) {
		error = Translate(_T("Select an extension"));
		return RequireLogin();
	}

	if (!api.GetSipConfig(extensionId, config)) {
		error = api.GetLastError();
		return ApiFailureResult(api);
	}

	SaveSelectedExtensionId(extensionId);
	return FourPhoneRestoreSuccess;
}

bool CFourPhoneProvisioning::ShowLogin(
	CWnd* parent,
	FourPhoneSipConfig& config,
	CString& extensionId)
{
	CFourPhoneLoginDlg dialog(parent);
	if (dialog.DoModal() != IDOK) {
		return false;
	}
	config = dialog.GetSipConfig();
	extensionId = dialog.GetSelectedExtensionId();
	return !config.extension.IsEmpty();
}

Account CFourPhoneProvisioning::BuildAccount(
	const FourPhoneSipConfig& config)
{
	Account account;
	account.label.Format(
		_T("4phone %s"),
		config.extension.GetString());
	account.server = config.sipServer;
	const bool useTls = config.transport == _T("tls");
	const int serverPort = useTls
		? config.sipTlsPort
		: config.sipPort;
	const int defaultPort = useTls ? 5061 : 5060;
	if (serverPort > 0 && serverPort != defaultPort) {
		account.server.AppendFormat(_T(":%d"), serverPort);
	}
	account.proxy.Empty();
	account.username = config.extension;
	account.domain = config.sipDomain;
	account.port = 0;
	account.authID = config.extension;
	account.password = config.password;
	account.rememberPassword = true;
	account.displayName = config.displayName;
	account.transport = config.transport;
	account.srtp.Empty();
	account.registerRefresh = 300;
	account.keepAlive = 15;
	account.publish = false;
	account.ice = false;
	account.allowRewrite = true;
	account.disableSessionTimer = false;
	return account;
}

bool CFourPhoneProvisioning::SaveAccount(
	const Account& value,
	CString& error)
{
	error.Empty();
	Account account = value;
	if (!accountSettings.AccountSave(1, &account)) {
		error = Translate(_T(
			"Windows could not protect the SIP password. "
			"Settings were not saved"));
		accountSettings.AccountDelete(1);
		return false;
	}
	if (!SetLoggedOut(false)) {
		error = Translate(
			_T("Could not save the 4phone sign-in state"));
		accountSettings.AccountDelete(1);
		return false;
	}
	accountSettings.accountId = 1;
	accountSettings.account = account;
	accountSettings.account.rememberPassword =
		account.rememberPassword;
	accountSettings.SettingsSave();
	return true;
}

bool CFourPhoneProvisioning::Logout(CString& error)
{
	error.Empty();
	const bool logoutMarkerSaved = SetLoggedOut(true);

	bool serverRevoked = true;
	CString refreshToken;
	CString loadError;
	if (CFourPhoneCredentialStore::LoadRefreshToken(
		refreshToken,
		loadError)) {
		CFourPhoneApi api;
		if (!api.Logout(refreshToken)) {
			serverRevoked = false;
			error = api.GetLastError();
		}
		SecureClear(refreshToken);
	}
	else if (!loadError.IsEmpty()) {
		serverRevoked = false;
		error = loadError;
	}

	CString credentialError;
	const bool credentialsDeleted =
		CFourPhoneCredentialStore::DeleteRefreshToken(
			credentialError);

	WritePrivateProfileString(
		kProvisioningSection,
		kEmailKey,
		NULL,
		accountSettings.iniFile);
	WritePrivateProfileString(
		kProvisioningSection,
		kExtensionIdKey,
		NULL,
		accountSettings.iniFile);

	accountSettings.AccountDelete(1);
	accountSettings.accountId = 0;
	accountSettings.account = Account();
	accountSettings.account.rememberPassword = false;
	accountSettings.SettingsSave();

	if (!credentialsDeleted) {
		if (!error.IsEmpty()) {
			error.Append(_T("\r\n"));
		}
		error.Append(credentialError);
	}
	if (!logoutMarkerSaved) {
		if (!error.IsEmpty()) {
			error.Append(_T("\r\n"));
		}
		error.Append(Translate(
			_T("Could not save the local sign-out state")));
	}
	return serverRevoked && credentialsDeleted && logoutMarkerSaved;
}

CString CFourPhoneProvisioning::LoadSelectedExtensionId()
{
	CString extensionId;
	LPTSTR buffer = extensionId.GetBuffer(128);
	GetPrivateProfileString(
		kProvisioningSection,
		kExtensionIdKey,
		_T(""),
		buffer,
		128,
		accountSettings.iniFile);
	extensionId.ReleaseBuffer();
	return extensionId;
}

void CFourPhoneProvisioning::SaveSelectedExtensionId(
	const CString& extensionId)
{
	WritePrivateProfileString(
		kProvisioningSection,
		kExtensionIdKey,
		extensionId,
		accountSettings.iniFile);
}

bool CFourPhoneProvisioning::IsLoggedOut()
{
	return GetPrivateProfileInt(
		kProvisioningSection,
		kLoggedOutKey,
		0,
		accountSettings.iniFile) != 0;
}

bool CFourPhoneProvisioning::SetLoggedOut(bool loggedOut)
{
	return WritePrivateProfileString(
		kProvisioningSection,
		kLoggedOutKey,
		loggedOut ? _T("1") : NULL,
		accountSettings.iniFile) != FALSE;
}
