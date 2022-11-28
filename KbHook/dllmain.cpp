// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#define APP_TITLE L"KbHook"

#include "../util.h"

static HWND mainWindow = NULL;


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		mainWindow = FindWindowW(MAIN_WINDOW_CLASS, NULL);
		if (!mainWindow) {
			SHOW_ERROR(L"No main window!");
			return FALSE;
		}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


extern "C" LRESULT WINAPI HookProc(int code, WPARAM wParam, LPARAM lParam) {
	if (code == HC_ACTION) {
		PostMessageW(mainWindow, UM_HOOK, wParam, lParam);
	}
	return CallNextHookEx(NULL, code, wParam, lParam);
}
