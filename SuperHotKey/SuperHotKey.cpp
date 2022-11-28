// SuperHotKey.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "SuperHotKey.h"

const wchar_t* APP_TITLE = NULL;

#include "../util.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	APP_TITLE = szTitle;
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SUPERHOTKEY));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

static HHOOK hHook = NULL;



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SUPERHOTKEY));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SUPERHOTKEY);
	wcex.lpszClassName = MAIN_WINDOW_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(MAIN_WINDOW_CLASS, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 600, 400, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

std::wstring debugMessage;
RECT rcClient = { 0 };

// HotKey is a hot key config.
struct HotKey {
	std::vector<WPARAM> vkCodes;	// The keys pressed together to trigger cmd.
	std::wstring cmd;			// The command to execute. 
};

static std::vector<HotKey> hotkeys;
static std::set<WPARAM> pressedKeys;	// The keys current pressed by user.

void ProcessKeyDownEvent();
void ReadConfig();

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case UM_HOOK: {
		const auto vk = wParam;
		const bool down = !(lParam >> 31 & 1);

		if (down) {
			pressedKeys.insert(vk);
		}
		else {
			pressedKeys.erase(vk);
		}
		std::wostringstream buf;
		std::for_each(pressedKeys.begin(), pressedKeys.end(), [&buf](const auto k)-> void {
			buf << k << L" ";
			});
		debugMessage = buf.str();
		OutputDebugStringW((debugMessage + L"\n").c_str());
		InvalidateRect(hWnd, NULL, TRUE);

		if (!down) {
			break;
		}

		ProcessKeyDownEvent();
		break;
	}
	case WM_CREATE: {
		ReadConfig();

		const auto hDll = LoadLibraryW(L"KbHook.dll");
		if (!hDll) {
			SHOW_LAST_ERROR();
			std::exit(1);
		}
		const auto pHookProc = (HOOKPROC)GetProcAddress(hDll, "HookProc");
		if (!pHookProc) {
			SHOW_LAST_ERROR();
			std::exit(1);
		}
		hHook = SetWindowsHookExW(WH_KEYBOARD, pHookProc, hDll, 0);
		if (!hHook) {
			SHOW_LAST_ERROR();
			std::exit(1);
		}
		break;
	}
	case WM_COMMAND: {
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId) {
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	}
	case WM_SIZE:
		GetClientRect(hWnd, &rcClient);
		break;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		DrawTextW(hdc, debugMessage.c_str(), (int)debugMessage.length(),
			&rcClient, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		if (hHook) {
			if (!UnhookWindowsHookEx(hHook)) {
				SHOW_LAST_ERROR();
			}
		}
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void ReadConfig() {
	wchar_t moduleFileName[1024] = {0};
	GetModuleFileNameW(NULL, moduleFileName, sizeof(moduleFileName) / sizeof(moduleFileName[0]));
	std::wstring configFileName(moduleFileName);
	const auto pos =configFileName.rfind('\\');
	if (pos != std::wstring::npos) {
		configFileName = configFileName.substr(0, pos + 1) + L"SuperHotKey.ini";
	}
	if (!PathFileExistsW(configFileName.c_str())) {
		WritePrivateProfileStringW(L"HotKey1", L"Keys", L"", configFileName.c_str());
		WritePrivateProfileStringW(L"HotKey1", L"Command", L"", configFileName.c_str());
		return;
	}
	wchar_t buf[1024] = { 0 };
	for (int i = 1; i <= 99; i++) {
		const auto configName = (std::wostringstream() << L"HotKey" << i).str();
		GetPrivateProfileStringW(configName.c_str(),
			L"Keys", L"", buf, sizeof(buf) / sizeof(buf[0]),
			configFileName.c_str());
		if (lstrlenW(buf) == 0) {
			continue;
		}
		HotKey hotkey;
		std::wstringstream stream(buf);
		std::wstring key;
		bool invalidKey = false;
		while(!stream.eof()) {
			std::getline(stream, key, L' ');
			if (key.empty()) {
				continue;
			}
			int k;
			try {
				k = std::stoi(key);
			} catch (std::invalid_argument) {
				invalidKey = true;
				break;
			}
			hotkey.vkCodes.push_back(k);
		}
		if (invalidKey) {
			continue;
		}

		GetPrivateProfileStringW(configName.c_str(),
			L"Command", L"", buf, sizeof(buf) / sizeof(buf[0]),
			configFileName.c_str());
		if (lstrlenW(buf) == 0) {
			continue;
		}
		hotkey.cmd = buf;
		hotkeys.push_back(hotkey);
	}
}

void ProcessKeyDownEvent() {
	std::for_each(hotkeys.begin(), hotkeys.end(),
		[](auto& hotkey)->void {
			if (hotkey.vkCodes.size() != pressedKeys.size()) {
				return;
			}
			bool allPressed = true;
			for (std::vector<WPARAM>::const_iterator k = hotkey.vkCodes.begin(); k != hotkey.vkCodes.end(); ++k) {
				if (pressedKeys.find(*k) == pressedKeys.end()) {
					allPressed = false;
					break;
				}
			}
			if (allPressed) {
				STARTUPINFO startup = { sizeof(startup), 0 };
				PROCESS_INFORMATION process = { 0 };
				std::vector<wchar_t> cmd(hotkey.cmd.length() + 1);
				std::copy(hotkey.cmd.begin(), hotkey.cmd.end(), cmd.begin());
				if (CreateProcessW(NULL, &cmd[0],
					NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL,
					&startup, &process)) {
					CloseHandle(process.hProcess);
					CloseHandle(process.hThread);
				}
				else {
					SHOW_LAST_ERROR();
				}
			}
		});
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
