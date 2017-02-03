// UnityWindowHack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"



BOOL CALLBACK EnumWindowsProc(_In_ HWND hWnd, _In_ LPARAM)
{
    TCHAR className[256];

    GetClassName(hWnd, className, ARRAYSIZE(className));

    if (_tcscmp(className, TEXT("UnityContainerWndClass")) == 0) {
        LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
        exStyle &= ~WS_EX_TOOLWINDOW;
        exStyle |=  WS_EX_APPWINDOW;
        SetWindowLong(hWnd, GWL_EXSTYLE, exStyle);
    }

    return TRUE;
}

int main()
{
    for (;;) {
        EnumWindows(EnumWindowsProc, NULL);
        Sleep(100);
    }
}
