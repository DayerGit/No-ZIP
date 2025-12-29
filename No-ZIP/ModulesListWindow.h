#pragma once
#include <Windows.h>
#include <unordered_map>
#include <string>
#include <commctrl.h>
#include <windowsx.h>
#include "RegHelper.h"

class ModulesListWindow {
public:
	ModulesListWindow(std::unordered_map<std::wstring, std::wstring>& settingsTable, Registry& reg, UINT DPI);
	void Show();
	~ModulesListWindow() = default;

private:
	struct WindowStruct {
		std::unordered_map<std::wstring, std::wstring>& settingsTable;
		Registry& reg;
		WNDPROC lpOldEditWindowProc;
		HWND hListView, hAddButton, hDeleteButton;
		BOOL isEdit;
		HFONT hFont;
		wchar_t* oldBuf;
		UINT DPI;

		WindowStruct(std::unordered_map<std::wstring, std::wstring>& st, Registry& r)
			: settingsTable(st), reg(r), lpOldEditWindowProc(nullptr),
			hListView(nullptr), hAddButton(nullptr), hDeleteButton(nullptr),
			isEdit(FALSE), hFont(nullptr), oldBuf(nullptr) {}
	} ws;

	HWND hWindow;

	static LRESULT WINAPI DefEditWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	static LRESULT WINAPI DefModulesListWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	enum class Controls {
		Add,
		Delete,
		Editable
	};
};