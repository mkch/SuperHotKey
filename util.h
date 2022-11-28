#pragma once

struct HOOK_MSG {
    int code;
    WPARAM wParam;
    LPARAM lParam;
};

static const UINT UM_HOOK = WM_USER + 1;
static const wchar_t* MAIN_WINDOW_CLASS = L"github.com/mkch/SuperHotKey#mainWindowClass";

#define _WT(str) L""##str
// Should be:
// #define W__FILE__ L""##__FILE__
// But the linter of VSCode does not like it(bug?).
#define W__FILE__ _WT(__FILE__)

#define SHOW_ERROR(err) ShowError(err, W__FILE__, __LINE__)
#define SHOW_LAST_ERROR() SHOW_ERROR(GetLastError())

inline void ShowError(const wchar_t* msg) {
    MessageBoxW(NULL, msg, APP_TITLE, MB_ICONERROR);
}

inline void ShowError(const wchar_t* msg, const wchar_t* file, int line) {
    wchar_t message[1024] = { 0 };
    StringCbPrintfW(message, sizeof message, L"%s:%d\n%s", file, line, msg);
    ShowError(message);
}

// Show a message box with the error description of lastError.
inline void ShowError(DWORD lastError, const wchar_t* file, int line) {
    wchar_t* msg = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, lastError, 0, (LPWSTR)&msg, 0, NULL);
    const std::wstring message = (std::wostringstream() << lastError << L": " << (msg ? msg : L"<unknown>")).str();
    LocalFree(msg);
    ShowError(message.c_str(), file, line);
}