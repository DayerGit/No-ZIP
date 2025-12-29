#include "ModulesListWindow.h"
#include "HelpersWinAPI.h"
#include "DPI.h"
#include <iostream>

ModulesListWindow::ModulesListWindow(std::unordered_map<std::wstring, std::wstring>& settingsTable, Registry& reg, UINT DPI) : ws(settingsTable, reg) {
    CreateClassExW((LPWSTR)L"Modules", (HBRUSH)GetStockObject(WHITE_BRUSH), IDC_ARROW, this->DefModulesListWindowProc, sizeof(&ws));

	this->hWindow = CreateWindowExW(WS_EX_ACCEPTFILES, L"Modules", L"Модули программы", WS_OVERLAPPEDWINDOW, ScaleForDPI(10, DPI), ScaleForDPI(10, DPI), ScaleForDPI(500, DPI), ScaleForDPI(250, DPI), 0, 0, 0, 0);
	
    ws.hListView = CreateWindowExW(NULL, WC_LISTVIEW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_NOCOLUMNHEADER, 0, ScaleForDPI(10, DPI), ScaleForDPI(845, DPI), ScaleForDPI(150, DPI), hWindow, NULL, NULL, NULL);
    ListView_SetExtendedListViewStyle(ws.hListView, LVS_EX_FULLROWSELECT);

    ws.hAddButton = CreateWindowExW(NULL, L"button", L"Добавить", WS_CHILD | WS_VISIBLE, ScaleForDPI(370, DPI), ScaleForDPI(170, DPI), ScaleForDPI(80, DPI), ScaleForDPI(30, DPI), hWindow, reinterpret_cast<HMENU>(Controls::Add), 0, 0);
	
    ws.hDeleteButton = CreateWindowExW(NULL, L"button", L"Удалить", WS_CHILD | WS_VISIBLE, ScaleForDPI(280, DPI), ScaleForDPI(170, DPI), ScaleForDPI(80, DPI), ScaleForDPI(30, DPI), hWindow, reinterpret_cast<HMENU>(Controls::Delete), 0, 0);
    EnableWindow(ws.hDeleteButton, FALSE);

    ws.reg = reg;
    ws.hFont = CreateFontW(ScaleForDPI(15, DPI), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    
    ws.DPI = DPI;
    
    SetWindowLongPtrW(hWindow, GWLP_USERDATA, (LONG_PTR)&ws);

    LVCOLUMNW col = { 0 };
    col.mask = LVCF_WIDTH | LVCF_FMT;
    col.fmt = LVCFMT_LEFT;

    col.cx = ScaleForDPI(250, DPI);
    ListView_InsertColumn(ws.hListView, 0, &col);
    ListView_InsertColumn(ws.hListView, 1, &col);

    LVITEMW item = { 0 };
    item.mask = LVIF_TEXT;
    for (auto& i : settingsTable) {
        item.pszText = (LPWSTR)i.first.c_str();

        ListView_InsertItem(ws.hListView, &item);
        ListView_SetItemText(ws.hListView, 0, 1, (LPWSTR)i.second.c_str());

    }
}

LRESULT WINAPI ModulesListWindow::DefEditWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    WindowStruct* ws = reinterpret_cast<WindowStruct*>(GetWindowLongPtrW(GetParent(hwnd), GWLP_USERDATA));

    switch (msg) {
    case WM_KEYDOWN: {
        if (!ws->isEdit) {
            int len = GetWindowTextLengthW(hwnd) + 1;
            ws->oldBuf = new wchar_t[len + 1];
            ws->oldBuf[len] = 0;

            GetWindowTextW(hwnd, ws->oldBuf, len);
            if (ws->oldBuf[0] != '\0') {
                ws->reg.DeleteKey(ws->oldBuf);
                ws->settingsTable.erase(ws->oldBuf);
            }
            ws->isEdit = TRUE;
        }
        switch (wparam) {
        case VK_RETURN: {
            SetFocus(GetParent(hwnd));
            break;
        }
        case VK_ESCAPE: {
            SetWindowTextW(hwnd, ws->oldBuf);
            SetFocus(GetParent(hwnd));
            break;
        }
        case 'A': {
            if (GetKeyState(VK_CONTROL) & 0x8000)
                SendMessageW(hwnd, EM_SETSEL, 0, -1);
            break;
        }
        }

        break;
    }
    case WM_DESTROY: {
        ws->isEdit = FALSE;
        delete[] ws->oldBuf;
        break;
    }
    }
    return CallWindowProcW(ws->lpOldEditWindowProc, hwnd, msg, wparam, lparam);;
}

LRESULT WINAPI ModulesListWindow::DefModulesListWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    WindowStruct* ws = reinterpret_cast<WindowStruct*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_COMMAND: {
        switch (LOWORD(wparam)) {
        case static_cast<int>(Controls::Editable): {
            if (HIWORD(wparam) == EN_KILLFOCUS) {
                HWND hEdit = (HWND)lparam;
                int iItem = (int)(INT_PTR)GetPropW(hEdit, L"ItemIndex");
                int iSubItem = (int)(INT_PTR)GetPropW(hEdit, L"SubItemIndex");

                wchar_t szNewText[MAX_PATH];
                GetWindowTextW(hEdit, szNewText, sizeof(szNewText) / sizeof(wchar_t));

                ListView_SetItemText(ws->hListView, iItem, iSubItem, szNewText);

                wchar_t formatName[MAX_PATH] = { 0 };
                ListView_GetItemText(ws->hListView, iItem, 0, formatName, MAX_PATH);

                wchar_t DLLForFormatName[MAX_PATH] = { 0 };
                ListView_GetItemText(ws->hListView, iItem, 1, DLLForFormatName, MAX_PATH);

                ws->settingsTable[formatName] = DLLForFormatName;
                for (auto i : ws->settingsTable) {
                    std::wcout << i.first << " " << i.second << std::endl;
                    std::cout << ws->reg.SetKeyDefaultValue(i.first.c_str(), i.second.c_str()) << std::endl;
                }

                EnableWindow(ws->hListView, TRUE);
                DestroyWindow(hEdit);
            }
            break;
        }
        case static_cast<int>(Controls::Add): {
            LVITEMW tempItem = { 0 };
            tempItem.mask = LVIF_TEXT;
            ListView_InsertItem(ws->hListView, &tempItem);
            break;
        }
        case static_cast<int>(Controls::Delete): {
            int objectForDelete = -1;
            while ((objectForDelete = ListView_GetNextItem(ws->hListView, -1, LVNI_SELECTED)) != -1) {
                wchar_t formatName[MAX_PATH] = { 0 };
                ListView_GetItemText(ws->hListView, objectForDelete, 0, formatName, MAX_PATH);

                ws->settingsTable.erase(formatName);
                ws->reg.DeleteKey(formatName);
                ListView_DeleteItem(ws->hListView, objectForDelete);
            }

            EnableWindow(ws->hDeleteButton, FALSE);
            break;
        }
        }
        break;
    }
    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lparam;
        switch (hdr->code) {
        case NM_DBLCLK: {
            LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)lparam;
            RECT rcSubItem;
            ListView_GetSubItemRect(hdr->hwndFrom, itemActivate->iItem, itemActivate->iSubItem, LVIR_LABEL, &rcSubItem);
            if (itemActivate->iItem >= 0) {

                HWND hEdit = CreateWindowExW(NULL, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT,
                    rcSubItem.left + ScaleForDPI(2, ws->DPI), rcSubItem.top + ScaleForDPI(8, ws->DPI), rcSubItem.right - rcSubItem.left, rcSubItem.bottom - rcSubItem.top,
                    hwnd, reinterpret_cast<HMENU>(Controls::Editable), 0, 0);
                SetWindowFont(hEdit, ws->hFont, FALSE);

                ws->lpOldEditWindowProc = (WNDPROC)SetWindowLongPtrW(hEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(DefEditWindowProc));
                EnableWindow(hdr->hwndFrom, FALSE);
                wchar_t szText[MAX_PATH] = { 0 };
                ListView_GetItemText(hdr->hwndFrom, itemActivate->iItem, itemActivate->iSubItem, szText, sizeof(szText) / sizeof(wchar_t));
                SetWindowTextW(hEdit, szText);

                SetPropW(hEdit, L"ItemIndex", (HANDLE)(INT_PTR)itemActivate->iItem);
                SetPropW(hEdit, L"SubItemIndex", (HANDLE)(INT_PTR)itemActivate->iSubItem);

                SetFocus(hEdit);
            }
            break;
        }
        case LVN_ITEMCHANGED: {
            int selectedItem = ListView_GetNextItem(ws->hListView, -1, LVNI_SELECTED);
            EnableWindow(ws->hDeleteButton, selectedItem != -1);
            break;
        }
        }
        break;
    }
    case WM_CLOSE: {
        ShowWindow(hwnd, SW_HIDE);
        break;
    }
    default: return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    return 0;
}

void ModulesListWindow::Show() {
    ShowWindow(this->hWindow, SW_SHOWNORMAL);
}