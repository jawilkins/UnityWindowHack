#include "stdafx.h"
#include "resource.h"
#include "UnityWindowHack.h"


static HMENU     m_hMenu;
static HINSTANCE m_hInstance;

static BOOL m_bUnityRunning;
static int  m_countUnityWindows;

BOOL IsUnityFloatingWindow(HWND hWnd)
{
    TCHAR className[256];

    GetClassName(hWnd, className, ARRAYSIZE(className));

    if (_tcscmp(className, TEXT("UnityContainerWndClass")) == 0) {
        m_bUnityRunning = TRUE;

        return !(GetWindowLong(hWnd, GWL_STYLE) & WS_SYSMENU);
    }
    else {
        return FALSE;
    }
}

void SetAppWindowStyle(HWND hWnd, BOOL bApp)
{
    LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

    if (( bApp && (exStyle & WS_EX_TOOLWINDOW)) ||
        (!bApp && (exStyle & WS_EX_APPWINDOW )))
    {
        const BOOL visible = IsWindowVisible(hWnd);

        if (visible)
            ShowWindow(hWnd, SW_HIDE);

        if (bApp) {
            exStyle &= ~WS_EX_TOOLWINDOW;
            exStyle |=  WS_EX_APPWINDOW;
        }
        else {
            exStyle &= ~WS_EX_APPWINDOW;
            exStyle |=  WS_EX_TOOLWINDOW;
        }

        SetWindowLong(hWnd, GWL_EXSTYLE, exStyle);

        if (visible)
            ShowWindow(hWnd, SW_SHOW);
    }
}

BOOL CALLBACK HackUnityWindow(_In_ HWND hWnd, _In_ LPARAM bHack)
{
    if (IsUnityFloatingWindow(hWnd)) {
        m_countUnityWindows++;

        TCHAR text[128] = TEXT("");

        if (bHack) {
            HWND hwndChild = NULL;
            bool first = true;
            for (;;) {
                hwndChild =
                    FindWindowEx(
                        hWnd,
                        hwndChild,
                        TEXT("UnityGUIViewWndClass"),
                        NULL);

                if (!hwndChild)
                    break;

                if (!first)
                    _tcscat_s(text, TEXT(" - "));
                else
                    first = false;

                TCHAR childText[128];
                GetWindowText(hwndChild, childText, 128);
                TCHAR* name = _tcsstr(childText, TEXT("."));
                _tcscat_s(text, name ? name+1 : childText);
            }

            if (text[0] == '\0')
                _tcscpy_s(text, TEXT("Unity"));
        }

        SetWindowText(hWnd, text);
        SetAppWindowStyle(hWnd, bHack);
    }

    return TRUE;
}

void MakeCommandLine(PTSTR pstrProposedValue, rsize_t count, PCTSTR pstrPathToExe, PCTSTR pstrArgs)
{
    _tcscpy_s(pstrProposedValue, count, TEXT("\""));
    _tcscat_s(pstrProposedValue, count, pstrPathToExe);
    _tcscat_s(pstrProposedValue, count, TEXT("\""));

    if (pstrArgs != NULL) {
        _tcscat_s(pstrProposedValue, count, TEXT(" "));
        _tcscat_s(pstrProposedValue, count, pstrArgs);
    }

}

template <rsize_t count>
inline void MakeCommandLine(TCHAR (&strProposedValue)[count], PCTSTR pstrPathToExe, PCTSTR pstrArgs)
{
    MakeCommandLine(strProposedValue, count, pstrPathToExe, pstrArgs);
}

static const TCHAR m_strRunKeyName[] =
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run");

BOOL IsProgramRegisteredForStartup(
    PCTSTR pstrAppName,
    PCTSTR pstrPathToExe,
    PCTSTR pstrArgs)
{
    TCHAR strValue[MAX_PATH];

    {
        LSTATUS lStatus;
        HKEY    hKey;

        lStatus =
            RegOpenKeyEx(
                HKEY_CURRENT_USER,
                m_strRunKeyName,
                0,
                KEY_READ,
                &hKey);

        if (lStatus != ERROR_SUCCESS)
            return FALSE;

        DWORD dwRegType = REG_SZ;
        DWORD dwSize    = sizeof(strValue);

        lStatus =
            RegGetValue(
                hKey,
                NULL,
                pstrAppName,
                RRF_RT_REG_SZ,
                &dwRegType,
                strValue,
                &dwSize);

        RegCloseKey(hKey);

        if (lStatus != ERROR_SUCCESS)
            return FALSE;
    }

    TCHAR strProposedValue[2*MAX_PATH];
    MakeCommandLine(strProposedValue, pstrPathToExe, pstrArgs);
    return _tcscmp(strValue, strProposedValue) == 0;
}

BOOL RegisterProgramForStartup(PCTSTR pstrAppName, PCTSTR pstrPathToExe, PCTSTR pstrArgs)
{
    HKEY hKey;
    LSTATUS lStatus;

    lStatus =
        RegCreateKeyEx(
            HKEY_CURRENT_USER,
            m_strRunKeyName,
            0,
            NULL,
            0,
            (KEY_WRITE | KEY_READ),
            NULL,
            &hKey,
            NULL);

    if (lStatus != ERROR_SUCCESS)
        return FALSE;

    TCHAR strProposedValue[2*MAX_PATH];
    MakeCommandLine(strProposedValue, pstrPathToExe, pstrArgs);

    DWORD dwSize = sizeof(TCHAR)*(_tcslen(strProposedValue)+1);

    lStatus =
        RegSetValueEx(
            hKey,
            pstrAppName,
            0,
            REG_SZ,
            reinterpret_cast<BYTE*>(strProposedValue),
            dwSize);

    RegCloseKey(hKey);

    return lStatus == ERROR_SUCCESS;
}

BOOL UnregisterProgramForStartup(PCTSTR pstrAppName)
{
    LSTATUS lStatus =
        RegDeleteKeyValue(
            HKEY_CURRENT_USER,
            m_strRunKeyName,
            pstrAppName);

    return lStatus == ERROR_SUCCESS;
}

static const TCHAR m_strClassName[] = TEXT("UnityWindowHack");

static TCHAR m_strPathToExe[MAX_PATH];

void InitRegisterProgram()
{
    GetModuleFileName(NULL, m_strPathToExe, ARRAYSIZE(m_strPathToExe));
}

void RegisterProgram()
{
    RegisterProgramForStartup(m_strClassName, m_strPathToExe, NULL);
}

void UnregisterProgram()
{
    UnregisterProgramForStartup(m_strClassName);
}

BOOL IsProgramRegistered()
{
    return IsProgramRegisteredForStartup(m_strClassName, m_strPathToExe, NULL);
}

void ToggleRunAtStartup()
{
    if (!IsProgramRegistered())
        RegisterProgram();
    else
        UnregisterProgram();
}

inline void UpdateRunAtStartupCheck()
{
    const UINT check =
        IsProgramRegistered() ? MF_CHECKED : MF_UNCHECKED;

    CheckMenuItem(
        m_hMenu,
        ID_CONTEXTMENU_RUNATSTARTUP,
        check);
}

static NOTIFYICONDATA m_niData;

void UpdateTooltip()
{
    if (m_bUnityRunning)
        _stprintf_s(
            m_niData.szTip,
            TEXT("Unity Window Hack\n%d window%s hacked."),
            m_countUnityWindows,
            m_countUnityWindows == 1 ? TEXT("") : TEXT("s"));
    else
        _tcscpy_s(
            m_niData.szTip,
            TEXT("Unity Window Hack\nUnity not detected."));
}

void AddNotificationIcon()
{
    UpdateTooltip();
    Shell_NotifyIcon(NIM_ADD, &m_niData);
}

const static UINT uID = 1;

void InitNotificationIcon(HWND hWnd)
{
    m_niData.cbSize           = sizeof(m_niData);
    m_niData.uVersion         = NOTIFYICON_VERSION_4;
    m_niData.uID              = uID;
    m_niData.uFlags           = NIF_ICON|NIF_MESSAGE|NIF_TIP|NIF_SHOWTIP;
    m_niData.hIcon            = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_UNITY_WINDOW_HACK));
    m_niData.hWnd             = hWnd;
    m_niData.uCallbackMessage = WM_APP;

    AddNotificationIcon();
    Shell_NotifyIcon(NIM_SETVERSION, &m_niData);
}

void UpdateNotificationIcon()
{
    UpdateTooltip();
    Shell_NotifyIcon(NIM_MODIFY, &m_niData);
}

static BOOL bPrevUnityRunning;
static int  countPrevUnityWindows;

void HackUnityWindows()
{
    bPrevUnityRunning     = m_bUnityRunning;
    countPrevUnityWindows = m_countUnityWindows;

    m_bUnityRunning     = false;
    m_countUnityWindows = 0;

    EnumWindows(HackUnityWindow, TRUE);
}

BOOL StatusChanged()
{
    return bPrevUnityRunning     != m_bUnityRunning     ||
           countPrevUnityWindows != m_countUnityWindows ||
           m_niData.szTip[0]       == '\0';
}

void About(HWND hWnd)
{
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
}

LRESULT CALLBACK WindowProc(
    _In_ HWND   hWnd,
    _In_ UINT   msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (msg) {
        case WM_TIMER:
        {
            HackUnityWindows();

            if (StatusChanged())
                UpdateNotificationIcon();

            return 0;
        }
        break;

        case WM_APP:
        {
            if (HIWORD(lParam) != uID)
                return DefWindowProc(hWnd, msg, wParam, lParam);

            switch (LOWORD(lParam)) {
                case NIN_KEYSELECT:
                case NIN_SELECT:
                case WM_CONTEXTMENU:
                {
                    POINT point;
                    point.x = GET_X_LPARAM(wParam);
                    point.y = GET_Y_LPARAM(wParam);

                    SetForegroundWindow(hWnd);

                    const int cmd =
                        TrackPopupMenuEx(
                            m_hMenu,
                            TPM_RIGHTBUTTON|TPM_RETURNCMD,
                            point.x,
                            point.y,
                            hWnd,
                            NULL);

                    if (cmd == 0)
                        Shell_NotifyIcon(NIM_SETFOCUS, &m_niData);
                    else
                        PostMessage(hWnd, WM_COMMAND, cmd, 0);

                    PostMessage(hWnd, WM_NULL, 0, 0);

                    return 0;
                }
                default:
                    return DefWindowProc(hWnd, msg, wParam, lParam);
            }
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case ID_CONTEXTMENU_ABOUT:
                    About(hWnd);
                    return 0;
                case ID_CONTEXTMENU_RUNATSTARTUP:
                    ToggleRunAtStartup();
                    UpdateRunAtStartupCheck();
                    return 0;
                case ID_CONTEXTMENU_EXIT:
                    PostQuitMessage(0);
                    return 0;
                default:
                    return DefWindowProc(hWnd, msg, wParam, lParam);
            }
        }
        break;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

void Shutdown()
{
    EnumWindows(HackUnityWindow, FALSE);
}

int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE,
    _In_ LPSTR,
    _In_ int)
{
    InitRegisterProgram();

    m_hInstance = hInstance;

    m_hMenu = LoadMenu(m_hInstance, MAKEINTRESOURCE(IDR_MENU));
    m_hMenu = GetSubMenu(m_hMenu, 0);
    UpdateRunAtStartupCheck();

    WNDCLASS wndClass;
    ZeroMemory(&wndClass, sizeof(wndClass));

    wndClass.hInstance     = m_hInstance;
    wndClass.lpfnWndProc   = WindowProc;
    wndClass.lpszClassName = m_strClassName;

    RegisterClass(&wndClass);

    HWND hWnd =
        CreateWindow(
            m_strClassName,
            TEXT("Unity Window Hack"),
            0,
            0, 0, 0, 0,
            HWND_MESSAGE,
            0,
            m_hInstance,
            0);

    InitNotificationIcon(hWnd);

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
