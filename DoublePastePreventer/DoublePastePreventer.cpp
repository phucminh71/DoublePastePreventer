// DoublePastePreventer.cpp : Defines the entry point for the application.
//
#pragma comment(lib, "comctl32.lib")

#include "framework.h"
#include <shellapi.h>
#include <commctrl.h>
#include "DoublePastePreventer.h"
#include <string>

#define WM_TRAYICON (WM_USER + 1)
#define MAX_LOADSTRING 30
#define REG_DIR L"Software\\DoublePastePreventer"
#define STARTUP_DIR L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define ENABLED_REG_NAME L"Enabled"
#define DIALOG_ON_STARTUP_REG_NAME L"Dialog on startup"
#define STARTUP_REG_NAME L"Run on startup"
#define NOTIF_REG_NAME L"Show notification when block"
#define BLOCK_TIME_REG_NAME L"Block time"
#define SMART_REG_NAME L"Smart blocking"

constexpr unsigned int values[] = { 100, 250, 500, 750, 1000, 1500, 2000, 2500, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};
int x, y, w, h;

// Global Variables:
NOTIFYICONDATA notifIconData;
HINSTANCE hInst;                                // current instance
HHOOK hHook;
HKEY hKey;
HWND hDlg = NULL;
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
WCHAR exePath[MAX_PATH];

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    SettingsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void                SetTrayIcon(HWND hWnd);
void                HideTrayIcon(HWND hWnd);
void                ShowTrayIcon(HWND hWnd);

bool enabled = true, dialogOnStartup = false, runOnStartup = true, notif = false, smartBlocking = false;
UINT blockTimeInMS = 1000;

DWORD lastSeq = GetClipboardSequenceNumber();
ULONGLONG lastPasteTime = GetTickCount64();
int pasteAttempts = 0;

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

    RegCreateKeyExW(HKEY_CURRENT_USER, REG_DIR, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (hDlg && IsDialogMessage(hDlg, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
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

    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    DWORD size = sizeof(DWORD);
    RegGetValueW(HKEY_CURRENT_USER, REG_DIR, BLOCK_TIME_REG_NAME, RRF_RT_REG_DWORD, NULL, &blockTimeInMS, &size);
    DWORD dEnabled = 1, dDialogOnStartup = 0, dRunOnStartup = 1, dNotif = 0, dSmart = 0;
    RegGetValueW(HKEY_CURRENT_USER, REG_DIR, ENABLED_REG_NAME, RRF_RT_REG_DWORD, NULL, &dEnabled, &size);
    RegGetValueW(HKEY_CURRENT_USER, REG_DIR, DIALOG_ON_STARTUP_REG_NAME, RRF_RT_REG_DWORD, NULL, &dDialogOnStartup, &size);
    RegGetValueW(HKEY_CURRENT_USER, REG_DIR, STARTUP_REG_NAME, RRF_RT_REG_DWORD, NULL, &dRunOnStartup, &size);
    RegGetValueW(HKEY_CURRENT_USER, REG_DIR, NOTIF_REG_NAME, RRF_RT_REG_DWORD, NULL, &dNotif, &size);
    RegGetValueW(HKEY_CURRENT_USER, REG_DIR, SMART_REG_NAME, RRF_RT_REG_DWORD, NULL, &dSmart, &size);
    enabled = (dEnabled != 0);
    dialogOnStartup = (dDialogOnStartup != 0);
    runOnStartup = (dRunOnStartup != 0);
    notif = (dNotif != 0);
    smartBlocking = (dSmart != 0);

    hDlg = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_FORMVIEW), hWnd, SettingsProc);

    RECT r;
    GetWindowRect(hDlg, &r);
    AdjustWindowRect(&r, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, TRUE);
    w = r.right - r.left, h = r.bottom - r.top;

    POINT pt;
    GetCursorPos(&pt);
    HMONITOR mnt = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mnti;
    mnti.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(mnt, &mnti);
    x = (mnti.rcMonitor.right + mnti.rcMonitor.left) / 2 - w / 2;
    y = (mnti.rcMonitor.bottom + mnti.rcMonitor.top) / 2 - h / 2;

    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);

    SetTrayIcon(hWnd);
    if (dialogOnStartup) {
        SetWindowPos(hWnd, HWND_TOP, x, y, w, h, SWP_SHOWWINDOW);
        ShowWindow(hWnd, nCmdShow);
        HideTrayIcon(hWnd);
    }
    else {
        ShowWindow(hWnd, SW_HIDE);
    }
    UpdateWindow(hWnd);

    notifIconData.cbSize = sizeof(NOTIFYICONDATA);
    notifIconData.hWnd = hWnd;
    notifIconData.uID = 1;
    notifIconData.uFlags = NIF_INFO;
    notifIconData.dwInfoFlags = NIIF_INFO;
    wcscpy_s(notifIconData.szInfoTitle, L"Paste blocked");
    wcscpy_s(notifIconData.szInfo, L"Double paste prevented");

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
    case WM_DESTROY:
        UnhookWindowsHookEx(hHook);
        DestroyWindow(hDlg);
        RegCloseKey(hKey);
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
            ShowTrayIcon(hWnd);
            ShowWindow(hWnd, SW_HIDE);
        }
        return 0;
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONUP) {
            SetWindowPos(hWnd, HWND_TOP, x, y, w, h, SWP_SHOWWINDOW);
            ShowWindow(hWnd, SW_RESTORE);
            UpdateWindow(hWnd);
            SetForegroundWindow(hWnd);
            HideTrayIcon(hWnd);
            SendDlgItemMessageW(hDlg, IDC_CHECK1, BM_SETCHECK, (enabled ? BST_CHECKED : BST_UNCHECKED), 0);
        }
        else if (lParam == WM_RBUTTONUP) {
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_ENABLED, IDM_TOGGLE_ITEM, (enabled ? L"Disable" : L"Enable"));
            AppendMenuW(hMenu, MF_ENABLED, IDM_EXIT_ITEM, L"Exit");
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
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
            PostMessage(hWnd, WM_NULL, 0, 0);
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
    ULONGLONG currentTime = GetTickCount64();
    DWORD currentSeq = GetClipboardSequenceNumber();
    bool matchPasted = (currentSeq == lastSeq);
    bool withinWindow = (currentTime - lastPasteTime < blockTimeInMS);
    if ((!matchPasted || !withinWindow) && smartBlocking) pasteAttempts = 0;
    if (smartBlocking) pasteAttempts++;
    if (withinWindow && matchPasted && (pasteAttempts == 2 || !smartBlocking)) {
        if (notif) Shell_NotifyIconW(NIM_MODIFY, &notifIconData);
        return 1;
    }
    lastPasteTime = currentTime;
    lastSeq = currentSeq;
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}
static int FindNearestValueIndex(unsigned int target) {
    int lo = 0, hi = sizeof(values) / sizeof(values[0]) - 1;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (values[mid] == target) return mid;
        if (values[mid] < target) lo = mid + 1;
        else hi = mid;
    }
    if (lo > 0 && abs((int)values[lo] - (int)target) > abs((int)values[lo - 1] - (int)target))
        lo--;
    return lo;
}
INT_PTR CALLBACK SettingsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hButton;
    int index;
    switch (uMsg) {
    case WM_INITDIALOG:
        SendDlgItemMessageW(hWnd, IDC_SLIDER1, TBM_SETRANGE, TRUE, MAKELPARAM(0, sizeof(values) / sizeof(values[0]) - 1));
        SendDlgItemMessageW(hWnd, IDC_SLIDER1, TBM_SETTICFREQ, 1, 0);
        SendDlgItemMessageW(hWnd, IDC_SLIDER1, TBM_SETPAGESIZE, 0, 1);
        hButton = GetDlgItem(hWnd, IDC_BUTTON1);
        if(enabled) SendDlgItemMessageW(hWnd, IDC_CHECK1, BM_SETCHECK, BST_CHECKED, 0);
        if(dialogOnStartup) SendDlgItemMessageW(hWnd, IDC_CHECK2, BM_SETCHECK, BST_CHECKED, 0);
        if(runOnStartup) SendDlgItemMessageW(hWnd, IDC_CHECK3, BM_SETCHECK, BST_CHECKED, 0);
        if(notif) SendDlgItemMessageW(hWnd, IDC_CHECK4, BM_SETCHECK, BST_CHECKED, 0);
        if(smartBlocking) SendDlgItemMessageW(hWnd, IDC_CHECK5, BM_SETCHECK, BST_CHECKED, 0);
        SetDlgItemInt(hWnd, IDC_EDIT1, blockTimeInMS, false);
        SendDlgItemMessageW(hWnd, IDC_SLIDER1, TBM_SETPOS, TRUE, FindNearestValueIndex(blockTimeInMS));
        EnableWindow(hButton, FALSE);
        break;
    case WM_HSCROLL:
        index = SendDlgItemMessageW(hWnd, IDC_SLIDER1, TBM_GETPOS, 0, 0);
        SetDlgItemInt(hWnd, IDC_EDIT1, values[index], false);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_EDIT1 && HIWORD(wParam) == EN_CHANGE) {
            UINT newV = GetDlgItemInt(hWnd, IDC_EDIT1, NULL, FALSE);
            if (newV < 50 || newV > 10000) {
                if (newV < 50) newV = 50;
                else if (newV > 10000) newV = 10000;
                SetDlgItemInt(hWnd, IDC_EDIT1, newV, false);
            }
            SendDlgItemMessageW(hWnd, IDC_SLIDER1, TBM_SETPOS, TRUE, FindNearestValueIndex(newV));
            EnableWindow(hButton, TRUE);
        }else if (LOWORD(wParam) == IDC_BUTTON1 && HIWORD(wParam) == BN_CLICKED){
            HKEY hStartupKey;
            blockTimeInMS = GetDlgItemInt(hWnd, IDC_EDIT1, NULL, FALSE);
            DWORD dEnabled = (enabled ? 1 : 0), dDialogOnStartup = (dialogOnStartup ? 1 : 0), dRunOnStartup = (runOnStartup ? 1 : 0), 
                dNotif = (notif ? 1 : 0), dSmart = (smartBlocking ? 1 : 0);
            RegSetValueExW(hKey, ENABLED_REG_NAME, 0, REG_DWORD, (const BYTE*)(&dEnabled), sizeof(DWORD));
            RegSetValueExW(hKey, DIALOG_ON_STARTUP_REG_NAME, 0, REG_DWORD, (const BYTE*)(&dDialogOnStartup), sizeof(DWORD));
            RegSetValueExW(hKey, STARTUP_REG_NAME, 0, REG_DWORD, (const BYTE*)(&dRunOnStartup), sizeof(DWORD));
            RegSetValueExW(hKey, NOTIF_REG_NAME, 0, REG_DWORD, (const BYTE*)(&dNotif), sizeof(DWORD));
            RegSetValueExW(hKey, SMART_REG_NAME, 0, REG_DWORD, (const BYTE*)(&dSmart), sizeof(DWORD));
            RegSetValueExW(hKey, BLOCK_TIME_REG_NAME, 0, REG_DWORD, (const BYTE*)(&blockTimeInMS), sizeof(DWORD));
            RegOpenKeyExW(HKEY_CURRENT_USER, STARTUP_DIR, 0, KEY_SET_VALUE, &hStartupKey);
            if (runOnStartup)
                RegSetValueExW(hStartupKey, L"DoublePastePreventer", 0, REG_SZ, (const BYTE*)exePath, (wcslen(exePath) + 1) * sizeof(WCHAR));
            else
                RegDeleteValueW(hStartupKey, L"DoublePastePreventer");
            RegCloseKey(hStartupKey);
            EnableWindow(hButton, FALSE);
        }
        else if (LOWORD(wParam) == IDC_CHECK1 && HIWORD(wParam) == BN_CLICKED) {
            enabled = (IsDlgButtonChecked(hWnd, IDC_CHECK1) == BST_CHECKED);
            EnableWindow(hButton, TRUE);
        }
        else if (LOWORD(wParam) == IDC_CHECK2 && HIWORD(wParam) == BN_CLICKED) {
            dialogOnStartup = (IsDlgButtonChecked(hWnd, IDC_CHECK2) == BST_CHECKED);
            EnableWindow(hButton, TRUE);
        }
        else if (LOWORD(wParam) == IDC_CHECK3 && HIWORD(wParam) == BN_CLICKED) {
            runOnStartup = (IsDlgButtonChecked(hWnd, IDC_CHECK3) == BST_CHECKED);
            EnableWindow(hButton, TRUE);
        }
        else if (LOWORD(wParam) == IDC_CHECK4 && HIWORD(wParam) == BN_CLICKED) {
            notif = (IsDlgButtonChecked(hWnd, IDC_CHECK4) == BST_CHECKED);
            EnableWindow(hButton, TRUE);
        }
        else if (LOWORD(wParam) == IDC_CHECK5 && HIWORD(wParam) == BN_CLICKED) {
            smartBlocking = (IsDlgButtonChecked(hWnd, IDC_CHECK5) == BST_CHECKED);
            pasteAttempts = 0;
            EnableWindow(hButton, TRUE);
        }
    }
    return FALSE;
}
void SetTrayIcon(HWND hWnd) {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_DOUBLEPASTEPREVENTER));
    wcscpy_s(nid.szTip, szTitle);
    Shell_NotifyIcon(NIM_ADD, &nid);
}
void HideTrayIcon(HWND hWnd) {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_STATE;
    nid.dwState = NIS_HIDDEN;
    nid.dwStateMask = NIS_HIDDEN;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}
void ShowTrayIcon(HWND hWnd) {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_STATE;
    nid.dwState = 0;
    nid.dwStateMask = NIS_HIDDEN;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}