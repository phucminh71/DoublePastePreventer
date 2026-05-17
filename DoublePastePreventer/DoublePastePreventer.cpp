// DoublePastePreventer.cpp : Defines the entry point for the application.
//
#pragma comment(lib, "comctl32.lib")

#include "framework.h"
#include <shellapi.h>
#include <commctrl.h>
#include "DoublePastePreventer.h"
#include <string>
#include <iostream>

#define WM_TRAYICON (WM_USER + 1)
#define MAX_LOADSTRING 100

bool enabled = true;
unsigned long blockTimeInMS = 1000;
constexpr unsigned int values[] = { 100, 250, 500, 750, 1000, 1500, 2000, 2500, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};

// Global Variables:
HINSTANCE hInst;                                // current instance
HHOOK hHook;
HWND hDlg = NULL;
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    SettingsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void                SetTrayIcon(HWND hWnd, bool add);

std::wstring pastedText;
ULONGLONG lastPasteTime = GetTickCount64();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DOUBLEPASTEPREVENTER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DOUBLEPASTEPREVENTER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (hDlg && IsDialogMessage(hDlg, &msg))
            continue;
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DOUBLEPASTEPREVENTER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_DOUBLEPASTEPREVENTER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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
    hInst = hInstance;

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;

    hDlg = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_FORMVIEW), hWnd, SettingsProc);

    RECT r;
    GetWindowRect(hDlg, &r);
    AdjustWindowRect(&r, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, TRUE);
    int w = r.right - r.left, h = r.bottom - r.top;

    POINT pt;
    GetCursorPos(&pt);
    HMONITOR mnt = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mnti;
    mnti.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(mnt, &mnti);
    int x = (mnti.rcMonitor.right + mnti.rcMonitor.left) / 2 - w / 2;
    int y = (mnti.rcMonitor.bottom + mnti.rcMonitor.top) / 2 - h / 2;
    SetWindowPos(hWnd, HWND_TOP, x, y, w, h, SWP_SHOWWINDOW);

    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

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
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        UnhookWindowsHookEx(hHook);
        DestroyWindow(hDlg);
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
            SetTrayIcon(hWnd, true);
            ShowWindow(hWnd, SW_HIDE);
        }
        return 0;
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONUP) {
            ShowWindow(hWnd, SW_SHOW);
            SetForegroundWindow(hWnd);  
            SetTrayIcon(hWnd, false);
        }
        else if (lParam == WM_RBUTTONUP) {
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_ENABLED, IDM_TOGGLE_ITEM, (enabled ? L"Disable" : L"Enable"));
            AppendMenuW(hMenu, MF_ENABLED, IDM_EXIT_ITEM, L"Exit");
            POINT pt;
            GetCursorPos(&pt);
            BOOL cmd = TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            if (cmd == IDM_EXIT_ITEM) {
                UnhookWindowsHookEx(hHook);
                DestroyWindow(hDlg);
                PostQuitMessage(0);
            }
            else if (cmd == IDM_TOGGLE_ITEM) {
                enabled = !enabled;
            }
            DestroyMenu(hMenu);
        }
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
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

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0 || wParam != WM_KEYDOWN || !enabled) return CallNextHookEx(hHook, nCode, wParam, lParam);
    bool vKeyDown = (reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam)->vkCode == 0x56), ctrlKeyDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if (!vKeyDown || !ctrlKeyDown) return CallNextHookEx(hHook, nCode, wParam, lParam);
    if(!OpenClipboard(NULL)) return CallNextHookEx(hHook, nCode, wParam, lParam);
    bool matchPasted = false;
    std::wstring currentClip;
    HANDLE rawData = GetClipboardData(CF_UNICODETEXT);
    if (rawData == NULL) {
        CloseClipboard();
        return CallNextHookEx(hHook, nCode, wParam, lParam);
    }
    LPVOID dataPtr = GlobalLock(rawData);
    ULONGLONG currentTime = GetTickCount64();
    if (dataPtr != NULL) {
        const wchar_t* raw = static_cast<const wchar_t*>(dataPtr);
        matchPasted = (wcscmp(pastedText.c_str(), raw) == 0);
        if (!matchPasted || currentTime - lastPasteTime >= blockTimeInMS)
            currentClip = raw;
        GlobalUnlock(rawData);
    }
    CloseClipboard();
    if (currentTime - lastPasteTime < blockTimeInMS && matchPasted)  return 1;
    lastPasteTime = currentTime;
    if(!currentClip.empty()) pastedText = currentClip;
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

INT_PTR CALLBACK SettingsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int index;
    switch (uMsg) {
    case WM_INITDIALOG:
        SendDlgItemMessageW(hWnd, IDC_SLIDER1, TBM_SETRANGE, TRUE, MAKELPARAM(0, sizeof(values) / sizeof(values[0]) - 1));
        SendDlgItemMessageW(hWnd, IDC_SLIDER1, TBM_SETTICFREQ, 1, 0);
        SendDlgItemMessageW(hWnd, IDC_SLIDER1, TBM_SETPOS, TRUE, 4);
        SetDlgItemInt(hWnd, IDC_EDIT1, values[4], false);
        break;
    case WM_HSCROLL:
        index = SendDlgItemMessageW(hWnd, IDC_SLIDER1, TBM_GETPOS, 0, 0);
        SetDlgItemInt(hWnd, IDC_EDIT1, values[index], false);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_EDIT1 && HIWORD(wParam) == EN_CHANGE) {
            UINT newV = GetDlgItemInt(hWnd, IDC_EDIT1, NULL, FALSE);
            int lo = 0, hi = sizeof(values) / sizeof(values[0]) - 1;
            while (lo < hi) {
                int mid = (lo + hi) / 2;
                if (values[mid] == newV) { lo = mid; break; }
                if (values[mid] < newV) lo = mid + 1;
                else hi = mid;
            }
            if (lo > 0 && abs((int)values[lo] - (int)newV) > abs((int)values[lo - 1] - (int)newV))
                lo--;
            SendDlgItemMessage(hWnd, IDC_SLIDER1, TBM_SETPOS, TRUE, lo);    
        }else if (LOWORD(wParam) == IDC_BUTTON1 && HIWORD(wParam) == BN_CLICKED){
            blockTimeInMS = GetDlgItemInt(hWnd, IDC_EDIT1, NULL, FALSE);
        }
    }
    return FALSE;
}

void SetTrayIcon(HWND hWnd, bool add) {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_DOUBLEPASTEPREVENTER));
    wcscpy_s(nid.szTip, szTitle);
    Shell_NotifyIcon(add ? NIM_ADD : NIM_DELETE, &nid);
}