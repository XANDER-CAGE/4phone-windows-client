#pragma once

#include <afxstr.h>
#include <windows.h>

#define WM_4PHONE_UPDATE_CAN_SHUTDOWN (WM_APP + 0x4F0)
#define WM_4PHONE_UPDATE_REQUEST_SHUTDOWN (WM_APP + 0x4F1)

class CFourPhoneUpdateService
{
public:
	static bool Initialize(
		HWND ownerWindow,
		const CString& interval,
		const CString& language);
	static void CheckForUpdates();
	static void Cleanup();

private:
	static int __cdecl CanShutdown();
	static void __cdecl RequestShutdown();
	static int CheckIntervalSeconds(const CString& interval);
};
