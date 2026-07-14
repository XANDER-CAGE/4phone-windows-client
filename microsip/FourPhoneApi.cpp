/*
 * 4phone provisioning support for MicroSIP.
 *
 * This file is distributed under the GNU General Public License,
 * version 2 or any later version.
 */

#include "stdafx.h"
#include "FourPhoneApi.h"

#include "define.h"
#include "json.h"
#include "langpack.h"
#include "MSIP.h"

#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

namespace
{
	const DWORD kMaxResponseSize = 2 * 1024 * 1024;

	class CWinHttpHandle
	{
	public:
		CWinHttpHandle()
			: handle(NULL)
		{
		}

		explicit CWinHttpHandle(HINTERNET value)
			: handle(value)
		{
		}

		~CWinHttpHandle()
		{
			if (handle) {
				WinHttpCloseHandle(handle);
			}
		}

		HINTERNET Get() const
		{
			return handle;
		}

		bool IsValid() const
		{
			return handle != NULL;
		}

	private:
		CWinHttpHandle(const CWinHttpHandle&);
		CWinHttpHandle& operator=(const CWinHttpHandle&);

		HINTERNET handle;
	};

	CString JsonString(const Json::Value& value)
	{
		if (!value.isString()) {
			return _T("");
		}
		return MSIP::Utf8DecodeUni(value.asCString());
	}

	CStringA JsonBody(const Json::Value& value)
	{
		Json::FastWriter writer;
		const std::string json = writer.write(value);
		return CStringA(json.data(), static_cast<int>(json.size()));
	}

	std::string Utf8String(const CString& value)
	{
		CString copy = value;
		const CStringA utf8 = MSIP::Utf8EncodeUni(copy);
		return std::string(utf8.GetString(), utf8.GetLength());
	}

	bool ParseJson(const CStringA& body, Json::Value& root)
	{
		Json::Reader reader;
		return reader.parse(
			body.GetString(),
			body.GetString() + body.GetLength(),
			root,
			false);
	}

	bool IsSixDigitCode(const CString& value)
	{
		if (value.GetLength() != 6) {
			return false;
		}
		for (int index = 0; index < value.GetLength(); index++) {
			if (value[index] < _T('0') || value[index] > _T('9')) {
				return false;
			}
		}
		return true;
	}

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
}

CFourPhoneApi::CFourPhoneApi()
	: baseUrl(_T(_GLOBAL_4PHONE_API_URL))
	, lastStatusCode(0)
{
}

CFourPhoneApi::CFourPhoneApi(const CString& baseUrlValue)
	: baseUrl(baseUrlValue)
	, lastStatusCode(0)
{
}

CFourPhoneApi::~CFourPhoneApi()
{
	SecureClear(accessToken);
}

bool CFourPhoneApi::Login(
	const CString& email,
	const CString& password,
	FourPhoneLoginResponse& response)
{
	lastError.Empty();
	lastStatusCode = 0;
	response = FourPhoneLoginResponse();

	Json::Value payload;
	payload["email"] = Utf8String(email);
	payload["password"] = Utf8String(password);

	HttpResponse httpResponse;
	if (!Request(
		_T("POST"),
		_T("/auth/login"),
		JsonBody(payload),
		false,
		httpResponse)) {
		return false;
	}

	Json::Value root;
	if (!ParseJson(httpResponse.body, root)) {
		lastError = Translate(_T("The server returned an invalid response"));
		return false;
	}

	response.requiresTwoFactor =
		root.isMember("requiresTwoFactor") &&
		root["requiresTwoFactor"].asBool();
	if (response.requiresTwoFactor) {
		response.challengeToken = JsonString(root["challengeToken"]);
		if (response.challengeToken.IsEmpty()) {
			lastError = Translate(
				_T("The server did not return a 2FA challenge token"));
			return false;
		}
		return true;
	}

	if (!ParseTokens(httpResponse.body, response.tokens)) {
		return false;
	}
	accessToken = response.tokens.accessToken;
	return true;
}

bool CFourPhoneApi::VerifyTwoFactor(
	const CString& challengeToken,
	const CString& verificationValue,
	FourPhoneAuthTokens& tokens)
{
	lastError.Empty();
	lastStatusCode = 0;
	tokens = FourPhoneAuthTokens();

	Json::Value payload;
	payload["challengeToken"] = Utf8String(challengeToken);
	if (IsSixDigitCode(verificationValue)) {
		payload["code"] = Utf8String(verificationValue);
	}
	else {
		payload["recoveryCode"] = Utf8String(verificationValue);
	}

	HttpResponse response;
	if (!Request(
		_T("POST"),
		_T("/auth/2fa/verify-login"),
		JsonBody(payload),
		false,
		response)) {
		return false;
	}
	if (!ParseTokens(response.body, tokens)) {
		return false;
	}
	accessToken = tokens.accessToken;
	return true;
}

bool CFourPhoneApi::Refresh(
	const CString& refreshToken,
	FourPhoneAuthTokens& tokens)
{
	lastError.Empty();
	lastStatusCode = 0;
	tokens = FourPhoneAuthTokens();

	Json::Value payload;
	payload["refreshToken"] = Utf8String(refreshToken);

	HttpResponse response;
	if (!Request(
		_T("POST"),
		_T("/auth/refresh"),
		JsonBody(payload),
		false,
		response)) {
		return false;
	}
	if (!ParseTokens(response.body, tokens)) {
		return false;
	}
	accessToken = tokens.accessToken;
	return true;
}

bool CFourPhoneApi::Logout(const CString& refreshToken)
{
	lastError.Empty();
	lastStatusCode = 0;

	Json::Value payload;
	payload["refreshToken"] = Utf8String(refreshToken);

	HttpResponse response;
	return Request(
		_T("POST"),
		_T("/auth/logout"),
		JsonBody(payload),
		false,
		response);
}

bool CFourPhoneApi::GetExtensions(
	std::vector<FourPhoneExtension>& extensions)
{
	lastError.Empty();
	lastStatusCode = 0;
	extensions.clear();

	HttpResponse response;
	if (!Request(
		_T("GET"),
		_T("/extensions/softphone/assigned"),
		CStringA(),
		true,
		response)) {
		return false;
	}

	Json::Value root;
	if (!ParseJson(response.body, root) || !root.isArray()) {
		lastError = Translate(
			_T("The server returned an invalid extension list"));
		return false;
	}

	for (Json::UInt index = 0; index < root.size(); index++) {
		const Json::Value& item = root[index];
		FourPhoneExtension extension;
		extension.id = JsonString(item["id"]);
		extension.number = JsonString(item["number"]);
		extension.displayName = JsonString(item["displayName"]);
		if (!extension.id.IsEmpty() && !extension.number.IsEmpty()) {
			extensions.push_back(extension);
		}
	}
	return true;
}

bool CFourPhoneApi::GetSipConfig(
	const CString& extensionId,
	FourPhoneSipConfig& config)
{
	lastError.Empty();
	lastStatusCode = 0;
	config = FourPhoneSipConfig();

	CString path;
	path.Format(
		_T("/extensions/softphone/config/%s"),
		extensionId.GetString());

	HttpResponse response;
	if (!Request(
		_T("GET"),
		path,
		CStringA(),
		true,
		response)) {
		return false;
	}

	Json::Value root;
	if (!ParseJson(response.body, root) || !root.isObject()) {
		lastError = Translate(
			_T("The server returned an invalid SIP configuration"));
		return false;
	}

	config.sipDomain = JsonString(root["sipDomain"]);
	config.sipServer = JsonString(root["sipServer"]);
	config.extension = JsonString(root["extension"]);
	config.password = JsonString(root["password"]);
	config.displayName = JsonString(root["displayName"]);
	config.transport = JsonString(root["transport"]);
	config.transport.MakeLower();
	if (config.transport != _T("udp") &&
		config.transport != _T("tcp") &&
		config.transport != _T("tls")) {
		config.transport = _T("udp");
	}
	if (root.isMember("sipPort") && root["sipPort"].isInt()) {
		config.sipPort = root["sipPort"].asInt();
	}
	if (root.isMember("sipTlsPort") && root["sipTlsPort"].isInt()) {
		config.sipTlsPort = root["sipTlsPort"].asInt();
	}

	if (config.sipDomain.IsEmpty() ||
		config.sipServer.IsEmpty() ||
		config.extension.IsEmpty() ||
		config.password.IsEmpty()) {
		lastError = Translate(
			_T("The SIP configuration is missing required fields"));
		return false;
	}
	return true;
}

void CFourPhoneApi::SetAccessToken(const CString& value)
{
	accessToken = value;
}

CString CFourPhoneApi::GetAccessToken() const
{
	return accessToken;
}

CString CFourPhoneApi::GetLastError() const
{
	return lastError;
}

DWORD CFourPhoneApi::GetLastStatusCode() const
{
	return lastStatusCode;
}

bool CFourPhoneApi::Request(
	const CString& method,
	const CString& path,
	const CStringA& body,
	bool authenticated,
	HttpResponse& response)
{
	response = HttpResponse();

	if (authenticated && accessToken.IsEmpty()) {
		lastError = Translate(
			_T("Your 4phone session is not authorized"));
		return false;
	}

	CString normalizedBaseUrl = baseUrl;
	normalizedBaseUrl.Trim();
	normalizedBaseUrl.TrimRight(_T('/'));
	CString normalizedPath = path;
	if (normalizedPath.IsEmpty() || normalizedPath[0] != _T('/')) {
		normalizedPath.Insert(0, _T('/'));
	}
	const CString url = normalizedBaseUrl + normalizedPath;

	URL_COMPONENTS components;
	ZeroMemory(&components, sizeof(components));
	components.dwStructSize = sizeof(components);
	components.dwSchemeLength = static_cast<DWORD>(-1);
	components.dwHostNameLength = static_cast<DWORD>(-1);
	components.dwUrlPathLength = static_cast<DWORD>(-1);
	components.dwExtraInfoLength = static_cast<DWORD>(-1);

	if (!WinHttpCrackUrl(url, 0, 0, &components)) {
		SetTransportError(::GetLastError());
		return false;
	}
	if (components.nScheme != INTERNET_SCHEME_HTTPS) {
		lastError = Translate(_T("The 4phone API must use HTTPS"));
		return false;
	}

	const CString host(
		components.lpszHostName,
		static_cast<int>(components.dwHostNameLength));
	CString requestPath(
		components.lpszUrlPath,
		static_cast<int>(components.dwUrlPathLength));
	if (components.dwExtraInfoLength > 0) {
		requestPath.Append(
			components.lpszExtraInfo,
			static_cast<int>(components.dwExtraInfoLength));
	}
	if (requestPath.IsEmpty()) {
		requestPath = _T("/");
	}

	CWinHttpHandle session(WinHttpOpen(
		_T("4phone Softphone/1.0"),
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS,
		0));
	if (!session.IsValid()) {
		SetTransportError(::GetLastError());
		return false;
	}

	WinHttpSetTimeouts(session.Get(), 10000, 10000, 15000, 20000);
#ifdef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
	DWORD secureProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
	WinHttpSetOption(
		session.Get(),
		WINHTTP_OPTION_SECURE_PROTOCOLS,
		&secureProtocols,
		sizeof(secureProtocols));
#endif

	CWinHttpHandle connection(WinHttpConnect(
		session.Get(),
		host,
		components.nPort,
		0));
	if (!connection.IsValid()) {
		SetTransportError(::GetLastError());
		return false;
	}

	LPCWSTR acceptTypes[] = { L"application/json", NULL };
	CWinHttpHandle request(WinHttpOpenRequest(
		connection.Get(),
		method,
		requestPath,
		NULL,
		WINHTTP_NO_REFERER,
		acceptTypes,
		WINHTTP_FLAG_SECURE));
	if (!request.IsValid()) {
		SetTransportError(::GetLastError());
		return false;
	}
	DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
	if (!WinHttpSetOption(
		request.Get(),
		WINHTTP_OPTION_REDIRECT_POLICY,
		&redirectPolicy,
		sizeof(redirectPolicy))) {
		SetTransportError(::GetLastError());
		return false;
	}

	CString headers = _T("Accept: application/json\r\n");
	if (!body.IsEmpty()) {
		headers.Append(_T("Content-Type: application/json; charset=utf-8\r\n"));
	}
	if (authenticated) {
		headers.AppendFormat(
			_T("Authorization: Bearer %s\r\n"),
			accessToken.GetString());
	}
	if (!WinHttpAddRequestHeaders(
		request.Get(),
		headers,
		static_cast<DWORD>(-1),
		WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE)) {
		SetTransportError(::GetLastError());
		return false;
	}

	LPVOID requestData = body.IsEmpty()
		? WINHTTP_NO_REQUEST_DATA
		: static_cast<LPVOID>(const_cast<LPSTR>(body.GetString()));
	const DWORD requestDataSize = static_cast<DWORD>(body.GetLength());
	if (!WinHttpSendRequest(
		request.Get(),
		WINHTTP_NO_ADDITIONAL_HEADERS,
		0,
		requestData,
		requestDataSize,
		requestDataSize,
		0)) {
		SetTransportError(::GetLastError());
		return false;
	}
	if (!WinHttpReceiveResponse(request.Get(), NULL)) {
		SetTransportError(::GetLastError());
		return false;
	}

	DWORD statusSize = sizeof(response.statusCode);
	if (!WinHttpQueryHeaders(
		request.Get(),
		WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX,
		&response.statusCode,
		&statusSize,
		WINHTTP_NO_HEADER_INDEX)) {
		SetTransportError(::GetLastError());
		return false;
	}
	lastStatusCode = response.statusCode;

	DWORD totalSize = 0;
	for (;;) {
		DWORD available = 0;
		if (!WinHttpQueryDataAvailable(request.Get(), &available)) {
			SetTransportError(::GetLastError());
			return false;
		}
		if (available == 0) {
			break;
		}
		if (totalSize > kMaxResponseSize ||
			available > kMaxResponseSize - totalSize) {
			lastError = Translate(_T("The 4phone API response is too large"));
			return false;
		}

		LPSTR buffer = response.body.GetBuffer(
			static_cast<int>(totalSize + available));
		DWORD downloaded = 0;
		const BOOL read = WinHttpReadData(
			request.Get(),
			buffer + totalSize,
			available,
			&downloaded);
		totalSize += downloaded;
		response.body.ReleaseBuffer(static_cast<int>(totalSize));
		if (!read) {
			SetTransportError(::GetLastError());
			return false;
		}
		if (downloaded == 0) {
			break;
		}
	}

	if (response.statusCode < 200 || response.statusCode >= 300) {
		return ParseError(response);
	}
	return true;
}

bool CFourPhoneApi::ParseTokens(
	const CStringA& body,
	FourPhoneAuthTokens& tokens)
{
	Json::Value root;
	if (!ParseJson(body, root)) {
		lastError = Translate(
			_T("The server returned an invalid authentication response"));
		return false;
	}
	tokens.accessToken = JsonString(root["accessToken"]);
	tokens.refreshToken = JsonString(root["refreshToken"]);
	if (!tokens.IsValid()) {
		lastError = Translate(
			_T("The server did not return authentication tokens"));
		return false;
	}
	return true;
}

bool CFourPhoneApi::ParseError(const HttpResponse& response)
{
	Json::Value root;
	CString message;
	if (ParseJson(response.body, root)) {
		const Json::Value& apiMessage = root["message"];
		if (apiMessage.isString()) {
			message = JsonString(apiMessage);
		}
		else if (apiMessage.isArray() && apiMessage.size() > 0) {
			message = JsonString(apiMessage[0u]);
		}
		if (message.IsEmpty()) {
			message = JsonString(root["error"]);
		}
	}

	if (response.statusCode == 400) {
		lastError = Translate(
			_T("Check the entered data and try again"));
	}
	else if (response.statusCode == 401) {
		lastError = Translate(_T(
			"Invalid email, password, or verification code"));
	}
	else if (response.statusCode == 403) {
		lastError = Translate(_T("Access to this extension is denied"));
	}
	else if (response.statusCode == 404) {
		lastError = Translate(
			_T("The selected extension is no longer available"));
	}
	else if (response.statusCode == 429) {
		lastError = Translate(_T("Too many attempts. Try again later"));
	}
	else if (response.statusCode >= 500) {
		lastError = Translate(_T(
			"4phone is temporarily unavailable. Try again later"));
	}
	else if (!message.IsEmpty()) {
		lastError = message;
	}
	else {
		lastError.Format(
			Translate(_T("The 4phone API returned HTTP error %lu")),
			response.statusCode);
	}
	return false;
}

void CFourPhoneApi::SetTransportError(DWORD errorCode)
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

	CString systemMessage;
	if (length && buffer) {
		systemMessage = buffer;
		systemMessage.Trim();
	}
	if (buffer) {
		LocalFree(buffer);
	}

	if (systemMessage.IsEmpty()) {
		lastError.Format(
			Translate(_T(
				"Could not connect to the 4phone API, error %lu")),
			errorCode);
	}
	else {
		lastError.Format(
			Translate(_T("Could not connect to the 4phone API: %s")),
			systemMessage.GetString());
	}
	lastStatusCode = 0;
}
