#include "stdafx.h"
#include "FourPhoneUpdateService.h"

#include "const.h"

#include <atomic>
#include <winsparkle/winsparkle.h>

namespace
{
	const char* const kAppcastUrl =
		"https://api.4phone.uz/api/client-updates/windows/appcast.xml";
	const char* const kEd25519PublicKey =
		"EHnmlFUf18mWMI0iXr6qdiP4P0O6+PRsz3jESqE2hRI=";

	std::atomic<HWND> gOwnerWindow(NULL);
	std::atomic<bool> gInitialized(false);
}

bool CFourPhoneUpdateService::Initialize(
	HWND ownerWindow,
	const CString& interval,
	const CString& language)
{
	if (ownerWindow == NULL || !::IsWindow(ownerWindow)) {
		return false;
	}
	if (gInitialized.load()) {
		return true;
	}

	gOwnerWindow.store(ownerWindow);
	win_sparkle_set_appcast_url(kAppcastUrl);
	if (!win_sparkle_set_eddsa_public_key(kEd25519PublicKey)) {
		gOwnerWindow.store(NULL);
		return false;
	}
	win_sparkle_set_app_details(
		_T(_GLOBAL_COMPANY),
		_T(_GLOBAL_NAME),
		_T(_GLOBAL_VERSION));
	win_sparkle_set_app_build_version(_T(_GLOBAL_BUILD_VERSION));

	const CStringA languageCode(language);
	if (!languageCode.IsEmpty()) {
		win_sparkle_set_lang(languageCode);
	}

	const bool everyLaunch =
		interval.CompareNoCase(_T("always")) == 0;
	const bool automatic =
		interval.CompareNoCase(_T("never")) != 0 && !everyLaunch;
	win_sparkle_set_automatic_check_for_updates(automatic ? 1 : 0);
	win_sparkle_set_update_check_interval(CheckIntervalSeconds(interval));
	win_sparkle_set_can_shutdown_callback(CanShutdown);
	win_sparkle_set_shutdown_request_callback(RequestShutdown);
	win_sparkle_init();
	gInitialized.store(true);

	if (everyLaunch) {
		win_sparkle_check_update_without_ui();
	}
	return true;
}

void CFourPhoneUpdateService::CheckForUpdates()
{
	if (gInitialized.load()) {
		win_sparkle_check_update_with_ui();
	}
}

void CFourPhoneUpdateService::Cleanup()
{
	if (!gInitialized.exchange(false)) {
		gOwnerWindow.store(NULL);
		return;
	}

	gOwnerWindow.store(NULL);
	win_sparkle_cleanup();
}

int __cdecl CFourPhoneUpdateService::CanShutdown()
{
	const HWND ownerWindow = gOwnerWindow.load();
	if (ownerWindow == NULL || !::IsWindow(ownerWindow)) {
		return FALSE;
	}

	DWORD_PTR result = FALSE;
	const LRESULT sent = ::SendMessageTimeout(
		ownerWindow,
		WM_4PHONE_UPDATE_CAN_SHUTDOWN,
		0,
		0,
		SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT,
		2000,
		&result);
	return sent != 0 && result != FALSE;
}

void __cdecl CFourPhoneUpdateService::RequestShutdown()
{
	const HWND ownerWindow = gOwnerWindow.load();
	if (ownerWindow != NULL && ::IsWindow(ownerWindow)) {
		::PostMessage(
			ownerWindow,
			WM_4PHONE_UPDATE_REQUEST_SHUTDOWN,
			0,
			0);
	}
}

int CFourPhoneUpdateService::CheckIntervalSeconds(
	const CString& interval)
{
	if (interval.CompareNoCase(_T("daily")) == 0) {
		return 24 * 60 * 60;
	}
	if (interval.CompareNoCase(_T("monthly")) == 0) {
		return 30 * 24 * 60 * 60;
	}
	if (interval.CompareNoCase(_T("quarterly")) == 0) {
		return 90 * 24 * 60 * 60;
	}
	if (interval.CompareNoCase(_T("always")) == 0) {
		return 60 * 60;
	}
	return 7 * 24 * 60 * 60;
}
