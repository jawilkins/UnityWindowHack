#include "stdafx.h"
#include "resource.h"


HMENU     hMenu;
HINSTANCE hInstance;

BOOL CALLBACK HackUnityWindow(_In_ HWND hWnd, _In_ LPARAM)
{
    TCHAR className[256];

    GetClassName(hWnd, className, ARRAYSIZE(className));

    if (_tcscmp(className, TEXT("UnityContainerWndClass")) == 0) {
        LONG style   = GetWindowLong(hWnd, GWL_STYLE);
        LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

        if (!(style & WS_SYSMENU)) {
            exStyle &= ~WS_EX_TOOLWINDOW;
            exStyle |=  WS_EX_APPWINDOW;
            SetWindowLong(hWnd, GWL_EXSTYLE, exStyle);

            TCHAR text[128] = TEXT("");
            HWND hwndChild = NULL;
            bool first = true;
            for (;;) {
                hwndChild = FindWindowEx(hWnd, hwndChild, TEXT("UnityGUIViewWndClass"), NULL);

                if (!hwndChild)
                    break;

                if (!first)
                    _tcscat_s(text, TEXT(" - "));

                TCHAR childText[128];
                GetWindowText(hwndChild, childText, 128);
                TCHAR* name = _tcsstr(childText, TEXT("."));

                _tcscat_s(text, name ? name+1 : childText);

                first = false;
            }

            if (text[0] == '\0')
                _tcscpy_s(text, TEXT("Unity"));

            SetWindowText(hWnd, text);

            const static DWORD swpFlags =
                SWP_ASYNCWINDOWPOS |
                SWP_DEFERERASE     |
                SWP_NOACTIVATE     |
                SWP_NOMOVE         |
                SWP_NOOWNERZORDER  |
                SWP_NOREDRAW       |
                SWP_NOREPOSITION   |
                SWP_NOSENDCHANGING |
                SWP_NOSIZE         |
                SWP_NOZORDER;

            SetWindowPos(hWnd, NULL, 0, 0, 0, 0, swpFlags);
        }
    }

    return TRUE;
}

BOOL CALLBACK UnhackUnityWindow(_In_ HWND hWnd, _In_ LPARAM)
{
    TCHAR className[256];

    GetClassName(hWnd, className, ARRAYSIZE(className));

    if (_tcscmp(className, TEXT("UnityContainerWndClass")) == 0) {
        LONG style   = GetWindowLong(hWnd, GWL_STYLE);
        LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

        if (!(style & WS_SYSMENU)) {
            exStyle &= ~WS_EX_APPWINDOW;
            exStyle |=  WS_EX_TOOLWINDOW;
            SetWindowLong(hWnd, GWL_EXSTYLE, exStyle);
            SetWindowText(hWnd, TEXT(""));
        }
    }

    return TRUE;
}

static const TCHAR strClassName[] = TEXT("UnityWindowHack");

BOOL IsProgramRegisteredForStartup(PCWSTR pszAppName)
{
    HKEY hKey = NULL;
    LONG lResult = 0;
    BOOL fSuccess = TRUE;
    DWORD dwRegType = REG_SZ;
    wchar_t szPathToExe[MAX_PATH]  = { };
    DWORD dwSize = sizeof(szPathToExe);

    lResult = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey);

    fSuccess = (lResult == 0);

    if (fSuccess)
    {
        lResult = RegGetValueW(hKey, NULL, pszAppName, RRF_RT_REG_SZ, &dwRegType, szPathToExe, &dwSize);
        fSuccess = (lResult == 0);
    }

    if (fSuccess)
    {
        fSuccess = (wcslen(szPathToExe) > 0) ? TRUE : FALSE;
    }

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }

    return fSuccess;
}

BOOL RegisterMyProgramForStartup(PCWSTR pszAppName, PCWSTR pathToExe, PCWSTR args)
{
    HKEY hKey = NULL;
    LONG lResult = 0;
    BOOL fSuccess = TRUE;
    DWORD dwSize;

    const size_t count = MAX_PATH*2;
    wchar_t szValue[count] = { };


    wcscpy_s(szValue, count, L"\"");
    wcscat_s(szValue, count, pathToExe);
    wcscat_s(szValue, count, L"\" ");

    if (args != NULL)
    {
        // caller should make sure "args" is quoted if any single argument has a space
        // e.g. (L"-name \"Mark Voidale\"");
        wcscat_s(szValue, count, args);
    }

    lResult = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, (KEY_WRITE | KEY_READ), NULL, &hKey, NULL);

    fSuccess = (lResult == 0);

    if (fSuccess)
    {
        dwSize = (wcslen(szValue)+1)*2;
        lResult = RegSetValueExW(hKey, pszAppName, 0, REG_SZ, (BYTE*)szValue, dwSize);
        fSuccess = (lResult == 0);
    }

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }

    return fSuccess;
}

BOOL UnregisterMyProgramForStartup(PCWSTR pszAppName)
{
    return 0 == RegDeleteKeyValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", pszAppName);
}

void RegisterProgram()
{
    wchar_t szPathToExe[MAX_PATH];

    GetModuleFileNameW(NULL, szPathToExe, MAX_PATH);
    RegisterMyProgramForStartup(strClassName, szPathToExe, L"");
}

void UnregisterProgram()
{
    UnregisterMyProgramForStartup(strClassName);
}

BOOL IsProgramRegistered()
{
    return IsProgramRegisteredForStartup(strClassName);
}

DWORD ToggleRunAtStartup()
{
    if (!IsProgramRegistered())
        RegisterProgram();
    else
        UnregisterProgram();

    return IsProgramRegistered() ? MF_CHECKED : MF_UNCHECKED;
}

LRESULT CALLBACK WindowProc(
    _In_ HWND   hWnd,
    _In_ UINT   msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (msg) {
        case WM_TIMER:
            EnumWindows(HackUnityWindow, NULL);
            return 0;

        case WM_APP:
            switch(lParam) {
                case WM_CONTEXTMENU:
                case WM_RBUTTONUP:
                {
                    POINT point;
                    GetCursorPos(&point);
                    SetForegroundWindow(hWnd);
                    TrackPopupMenuEx(hMenu, TPM_RIGHTBUTTON, point.x, point.y, hWnd, NULL);
                    PostMessage(hWnd, WM_NULL, 0, 0);
                    return 0;
                }
                default:
                    return DefWindowProc(hWnd, msg, wParam, lParam);
            }

        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case ID_CONTEXTMENU_RUNATSTARTUP:
                    CheckMenuItem(hMenu, ID_CONTEXTMENU_RUNATSTARTUP, ToggleRunAtStartup());
                    return 0;
                case ID_CONTEXTMENU_ABOUT:
                    MessageBox(
                        hWnd,
                        TEXT(
                            "This program forces floating Unity3D windows\n"
                            "to have entries on the taskbar.\n"
                            "\n"
                            "License for application Icon Creative Commons Attribution 3.0\n"
                            "https://www.designcontest.com/free-outline-icons/"),
                        TEXT("Unity Window Hack"),
                        MB_OK);
                    return 0;
                case ID_EXIT:
                    PostQuitMessage(0);
                    return 0;
                default:
                    return DefWindowProc(hWnd, msg, wParam, lParam);
            }

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

void Shutdown()
{
    EnumWindows(UnhackUnityWindow, NULL);
}

int CALLBACK WinMain(
    _In_ HINSTANCE hCurrentInstance,
    _In_ HINSTANCE,
    _In_ LPSTR,
    _In_ int)
{
    hInstance = hCurrentInstance;

    hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU));
    hMenu = GetSubMenu(hMenu, 0);

    CheckMenuItem(hMenu, ID_CONTEXTMENU_RUNATSTARTUP, IsProgramRegistered() ? MF_CHECKED : MF_UNCHECKED);

    WNDCLASS wndClass;
    ZeroMemory(&wndClass, sizeof(wndClass));

    wndClass.hInstance     = hInstance;
    wndClass.lpfnWndProc   = WindowProc;
    wndClass.lpszClassName = strClassName;

    RegisterClass(&wndClass);

    HWND hWnd =
        CreateWindow(
            strClassName,
            TEXT("Unity Window Hack"),
            0,
            0, 0, 0, 0,
            HWND_MESSAGE,
            0,
            hInstance,
            0);

    NOTIFYICONDATA niData;
    ZeroMemory(&niData, sizeof(niData));

    niData.cbSize           = sizeof(niData);
    niData.uID              = 0;
    niData.uFlags           = NIF_ICON|NIF_MESSAGE|NIF_TIP;
    niData.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_UNITY_WINDOW_HACK));
    niData.hWnd             = hWnd;
    niData.uCallbackMessage = WM_APP;

    _tcscpy_s(niData.szTip, TEXT("Unity Window Hack"));

    Shell_NotifyIcon(NIM_ADD, &niData);

    SetTimer(hWnd, 1, 100, NULL);

    atexit(Shutdown);

    for(;;) {
        MSG msg;
        while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return msg.wParam;
            }
            else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        WaitMessage();
    }
}
