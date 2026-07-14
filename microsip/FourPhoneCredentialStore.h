/*
 * 4phone provisioning support for MicroSIP.
 *
 * This file is distributed under the GNU General Public License,
 * version 2 or any later version.
 */

#pragma once

#include "stdafx.h"

class CFourPhoneCredentialStore
{
public:
	static bool SaveRefreshToken(
		const CString& refreshToken,
		CString& error);
	static bool LoadRefreshToken(
		CString& refreshToken,
		CString& error);
	static bool DeleteRefreshToken(CString& error);

private:
	static CString FormatWindowsError(
		const CString& action,
		DWORD errorCode);
};
