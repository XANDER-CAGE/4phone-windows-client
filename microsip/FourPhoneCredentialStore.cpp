/*
 * 4phone provisioning support for MicroSIP.
 *
 * This file is distributed under the GNU General Public License,
 * version 2 or any later version.
 */

#include "stdafx.h"
#include "FourPhoneCredentialStore.h"

#include "define.h"
#include "langpack.h"

#include <wincred.h>

#pragma comment(lib, "advapi32.lib")

bool CFourPhoneCredentialStore::SaveRefreshToken(
	const CString& refreshToken,
	CString& error)
{
	error.Empty();
	if (refreshToken.IsEmpty()) {
		error = Translate(_T("Cannot save an empty 4phone token"));
		return false;
	}

	const DWORD blobSize =
		static_cast<DWORD>(refreshToken.GetLength() * sizeof(TCHAR));
	if (blobSize > CRED_MAX_CREDENTIAL_BLOB_SIZE) {
		error = Translate(_T(
			"4phone token exceeds the Windows credential size limit"));
		return false;
	}

	CREDENTIAL credential;
	ZeroMemory(&credential, sizeof(credential));
	credential.Type = CRED_TYPE_GENERIC;
	credential.TargetName =
		const_cast<LPTSTR>(_T(_GLOBAL_4PHONE_CREDENTIAL_TARGET));
	credential.CredentialBlobSize = blobSize;
	credential.CredentialBlob = reinterpret_cast<LPBYTE>(
		const_cast<LPTSTR>(refreshToken.GetString()));
	credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
	credential.UserName = const_cast<LPTSTR>(_T("4phone"));

	if (!CredWrite(&credential, 0)) {
		error = FormatWindowsError(
			Translate(_T("Could not save the 4phone session")),
			GetLastError());
		return false;
	}
	return true;
}

bool CFourPhoneCredentialStore::LoadRefreshToken(
	CString& refreshToken,
	CString& error)
{
	refreshToken.Empty();
	error.Empty();

	PCREDENTIAL credential = NULL;
	if (!CredRead(
		_T(_GLOBAL_4PHONE_CREDENTIAL_TARGET),
		CRED_TYPE_GENERIC,
		0,
		&credential)) {
		const DWORD errorCode = GetLastError();
		if (errorCode == ERROR_NOT_FOUND) {
			return false;
		}
		error = FormatWindowsError(
			Translate(_T("Could not read the 4phone session")),
			errorCode);
		return false;
	}

	if (credential->CredentialBlob &&
		credential->CredentialBlobSize > 0 &&
		credential->CredentialBlobSize % sizeof(TCHAR) == 0) {
		refreshToken.SetString(
			reinterpret_cast<LPCTSTR>(credential->CredentialBlob),
			static_cast<int>(
				credential->CredentialBlobSize / sizeof(TCHAR)));
	}
	if (credential->CredentialBlob &&
		credential->CredentialBlobSize > 0) {
		SecureZeroMemory(
			credential->CredentialBlob,
			credential->CredentialBlobSize);
	}
	CredFree(credential);

	if (refreshToken.IsEmpty()) {
		error = Translate(_T("Saved 4phone session is corrupted"));
		return false;
	}
	return true;
}

bool CFourPhoneCredentialStore::DeleteRefreshToken(CString& error)
{
	error.Empty();
	if (CredDelete(
		_T(_GLOBAL_4PHONE_CREDENTIAL_TARGET),
		CRED_TYPE_GENERIC,
		0)) {
		return true;
	}

	const DWORD errorCode = GetLastError();
	if (errorCode == ERROR_NOT_FOUND) {
		return true;
	}
	error = FormatWindowsError(
		Translate(_T("Could not delete the 4phone session")),
		errorCode);
	return false;
}

CString CFourPhoneCredentialStore::FormatWindowsError(
	const CString& action,
	DWORD errorCode)
{
	LPTSTR buffer = NULL;
	const DWORD length = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		0,
		reinterpret_cast<LPTSTR>(&buffer),
		0,
		NULL);

	CString message = action;
	if (length && buffer) {
		CString details = buffer;
		details.Trim();
		message.AppendFormat(_T(": %s"), details.GetString());
	}
	else {
		message.AppendFormat(Translate(_T(", error %lu")), errorCode);
	}
	if (buffer) {
		LocalFree(buffer);
	}
	return message;
}
