#include "HelpersWinAPI.h"
#include <stdio.h>

ATOM CreateClassExW(LPWSTR className, HBRUSH backBrush, LPCWSTR cursor, WNDPROC wndProc, size_t extraSize) {
	WNDCLASSEXW wcl = { 0 };
	wcl.cbSize = sizeof(wcl);
	wcl.lpszClassName = className;
	wcl.hbrBackground = backBrush;
	wcl.hCursor = LoadCursorW(NULL, cursor);
	wcl.lpfnWndProc = wndProc;
	wcl.cbWndExtra = extraSize;

	return RegisterClassExW(&wcl);
}

void DisplayError() {
	wchar_t buf[MAX_PATH] = { 0 };
	swprintf_s(buf, MAX_PATH, L"Ошибка: 0x%X", GetLastError());
	MessageBoxW(0, buf, L"Ошибка!", MB_ICONERROR);
}

void DisplayErrorForCode(LRESULT res) {
	wchar_t buf[MAX_PATH] = { 0 };
	swprintf_s(buf, MAX_PATH, L"Ошибка: 0x%X", res);
	MessageBoxW(0, buf, L"Ошибка!", MB_ICONERROR);
}