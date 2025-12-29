#pragma once
#include <Windows.h>

ATOM CreateClassExW(LPWSTR className, HBRUSH backBrush, LPCWSTR cursor, WNDPROC wndProc, size_t extraSize);
void DisplayError();
void DisplayErrorForCode(LRESULT res);