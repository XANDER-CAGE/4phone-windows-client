/*
 * 4phone provisioning support for MicroSIP.
 *
 * This file is distributed under the GNU General Public License,
 * version 2 or any later version.
 */

#pragma once

#include "stdafx.h"

#include <vector>

struct FourPhoneAuthTokens
{
	CString accessToken;
	CString refreshToken;

	bool IsValid() const
	{
		return !accessToken.IsEmpty() && !refreshToken.IsEmpty();
	}
};

struct FourPhoneLoginResponse
{
	bool requiresTwoFactor;
	CString challengeToken;
	FourPhoneAuthTokens tokens;

	FourPhoneLoginResponse()
		: requiresTwoFactor(false)
	{
	}
};

struct FourPhoneExtension
{
	CString id;
	CString number;
	CString displayName;
};

struct FourPhoneSipConfig
{
	CString sipDomain;
	CString sipServer;
	int sipPort;
	int sipTlsPort;
	CString extension;
	CString password;
	CString displayName;
	CString transport;

	FourPhoneSipConfig()
		: sipPort(5060)
		, sipTlsPort(5061)
		, transport(_T("udp"))
	{
	}
};

class CFourPhoneApi
{
public:
	CFourPhoneApi();
	explicit CFourPhoneApi(const CString& baseUrl);
	~CFourPhoneApi();

	bool Login(
		const CString& email,
		const CString& password,
		FourPhoneLoginResponse& response);
	bool VerifyTwoFactor(
		const CString& challengeToken,
		const CString& verificationValue,
		FourPhoneAuthTokens& tokens);
	bool Refresh(
		const CString& refreshToken,
		FourPhoneAuthTokens& tokens);
	bool Logout(const CString& refreshToken);
	bool GetExtensions(std::vector<FourPhoneExtension>& extensions);
	bool GetSipConfig(
		const CString& extensionId,
		FourPhoneSipConfig& config);

	void SetAccessToken(const CString& accessToken);
	CString GetAccessToken() const;
	CString GetLastError() const;
	DWORD GetLastStatusCode() const;

private:
	struct HttpResponse
	{
		DWORD statusCode;
		CStringA body;

		HttpResponse()
			: statusCode(0)
		{
		}
	};

	bool Request(
		const CString& method,
		const CString& path,
		const CStringA& body,
		bool authenticated,
		HttpResponse& response);
	bool ParseTokens(const CStringA& body, FourPhoneAuthTokens& tokens);
	bool ParseError(const HttpResponse& response);
	void SetTransportError(DWORD errorCode);

	CString baseUrl;
	CString accessToken;
	CString lastError;
	DWORD lastStatusCode;
};
