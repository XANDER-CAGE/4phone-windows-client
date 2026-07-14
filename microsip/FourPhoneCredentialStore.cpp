/*
 * 4phone provisioning support for MicroSIP.
 *
 * This file is distributed under the GNU General Public License,
 * version 2 or any later version.
 */

#include "stdafx.h"
#include "FourPhoneCredentialStore.h"

#include "define.h"

#include <wincred.h>

#pragma comment(lib, "advapi32.lib")

bool CFourPhoneCredentialStore::SaveRefreshToken(
	const CString& refreshToken,
	CString& error)
{
	error.Empty();
	if (refreshToken.IsEmpty()) {
		error = _T("Нельзя сохранить пустой токен 4phone");
		return false;
	}

	const DWORD blobSize =
		static_cast<DWORD>(refreshToken.GetLength() * sizeof(TCHAR));
	if (blobSize > CRED_MAX_CREDENTIAL_BLOB_SIZE) {
		error = _T("Токен 4phone превышает лимит хранилища Windows");
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
			_T("Не удалось сохранить сеанс 4phone"),
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
			_T("Не удалось прочитать сеанс 4phone"),
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
		error = _T("Сохраненный сеанс 4phone поврежден");
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
		_T("Не удалось удалить сеанс 4phone"),
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
		message.AppendFormat(_T(", код %lu"), errorCode);
	}
	if (buffer) {
		LocalFree(buffer);
	}
	return message;
}
