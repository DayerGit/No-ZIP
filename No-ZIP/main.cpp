#define _CRT_SECURE_NO_WARNINGS

#include <unordered_map>
#include <iostream>
#include <shlwapi.h>
#include <sstream>
#include <shlobj.h>
#include "HelpersWinAPI.h"
#include "DPI.h"
#include "RegHelper.h"
#include "DeclarationOnWorkWithArchives.h"
#include "constants.h"
#include "ModulesListWindow.h"

std::unordered_map<std::wstring, std::wstring> settingsTable;
UINT currentDPI;

struct FileInfo {
    HWND hListView;
    FileToken token;
    std::wstring extension = L"";
};

struct WindowLocalStruct {
    ModulesListWindow* modulesListWindow;
    std::unordered_map<std::wstring, loadingLibrary>& operationForFileType;

    size_t fileCount = 0;
    HWND hTabControl;
    HMENU hMenuFile;
    std::vector<FileInfo> vectorFilesInfo;
    HFONT hFontForCloseTab;
    LVCOLUMNW col = { LVCF_WIDTH | LVCF_FMT, LVCFMT_LEFT };
};

void OpenFileForArchive(const wchar_t* buf, WindowLocalStruct* wls, HWND hwnd) {
    LPWSTR extension = PathFindExtensionW(buf);
    LPWSTR fileName = PathFindFileNameW(buf);
    TCITEMW tcItem;
    tcItem.mask = TCIF_TEXT;
    try {
        auto it = wls->operationForFileType.find(extension);
        if (it != wls->operationForFileType.end()) {
            FileInfo temp;
            temp.token = wls->operationForFileType[extension].OpenArchive(buf);
            temp.extension = extension;
            if (temp.token >= 0) {
                temp.hListView = CreateWindowExW(NULL, WC_LISTVIEWW, L"List", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_NOCOLUMNHEADER, 0, ScaleForDPI(35, currentDPI), ScaleForDPI(620, currentDPI), ScaleForDPI(370, currentDPI), hwnd, 0, 0, 0);
                wls->col.cx = ScaleForDPI(500, currentDPI);
                ListView_InsertColumn(temp.hListView, 0, &wls->col);

                tcItem.pszText = fileName;
                TabCtrl_InsertItem(wls->hTabControl, TabCtrl_GetItemCount(wls->hTabControl), &tcItem);

                size_t size = wls->operationForFileType[extension].GetFileCount(temp.token);
                if (size >= 1) {
                    auto files = wls->operationForFileType[extension].GetListOfFiles(temp.token);
                    if (!files) {
                        MessageBoxW(NULL, L"Ошибка обработки архива!", L"Ошибка!", MB_OK | MB_ICONERROR);
                        wls->operationForFileType[extension].CloseFile(temp.token);
                        return;
                    }

                    LVITEMW item = { 0 };
                    item.mask = LVIF_TEXT;
                    for (size_t j = 0; j < size; j++) {
                        wchar_t fileNameInArchive[MAX_PATH] = { 0 };
                        MultiByteToWideChar(CP_ACP, 0, files[j].fileName, MAX_PATH, fileNameInArchive, MAX_PATH);
                        item.pszText = fileNameInArchive;
                        ListView_InsertItem(temp.hListView, &item);
                    }

                    TabCtrl_SetCurSel(wls->hTabControl, TabCtrl_GetItemCount(wls->hTabControl) - 1);
                    wls->vectorFilesInfo.emplace_back(temp);
                }
                else {
                    MessageBoxW(NULL, L"Пустой архив!", L"Ошибка!", MB_OK | MB_ICONERROR);
                    wls->operationForFileType[extension].CloseFile(temp.token);
                }
            }
            else DisplayError();
        }
        else MessageBoxW(NULL, L"Тип файла не зарегистрирован!", L"Ошибка!", MB_OK | MB_ICONERROR);
    }
    catch (std::bad_function_call& ex) {
        MessageBoxW(NULL, L"Тип файла не зарегистрирован!", L"Ошибка!", MB_OK | MB_ICONERROR);
    }
}

std::vector<int> GetSelectedItems(HWND hListView) {
    std::vector<int> selectedItems;

    int i = -1;
    while ((i = ListView_GetNextItem(hListView, i, LVNI_SELECTED)) != -1) {
        selectedItems.push_back(i);
    }

    return selectedItems;
}

LRESULT WINAPI DefZIPWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    WindowLocalStruct* wls = (WindowLocalStruct*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (msg) {
    case WM_DROPFILES: {
        wchar_t buf[MAX_PATH] = { 0 };
        UINT countOfFiles = DragQueryFileW(reinterpret_cast<HDROP>(wparam), UINT_MAX, NULL, NULL);
        wls->vectorFilesInfo.reserve( wls->vectorFilesInfo.capacity() + countOfFiles);
        wls->fileCount += countOfFiles;

        for (UINT i = 0; i < countOfFiles; i++) {
            memset(buf, 0, MAX_PATH);
            DragQueryFileW(reinterpret_cast<HDROP>(wparam), i, buf, MAX_PATH);
            OpenFileForArchive(buf, wls, hwnd);
        }
        break;
    }
    case WM_COMMAND: {
        switch (wparam) {
        case static_cast<UINT_PTR>(Menu::File_Open): {
            wchar_t path[MAX_PATH] = { 0 };

            OPENFILENAMEW openFile = { 0 };
            openFile.lStructSize = sizeof(OPENFILENAMEW);
            openFile.hwndOwner = hwnd;

            std::wstring filterString;

            for (auto& i : settingsTable) {
                filterString += i.first;
                filterString += L'\0';        
                filterString += L'*';        
                filterString += i.first;
                filterString += L'\0';        
            }
            filterString += L'\0';            

            openFile.lpstrFilter = filterString.c_str(); 
            openFile.lpstrFile = path;
            openFile.nMaxFile = MAX_PATH;
            openFile.lpstrTitle = L"Открыть файл"; 
            openFile.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

            if (GetOpenFileNameW(&openFile))
                OpenFileForArchive(path, wls, hwnd);
            break;
        }
        case static_cast<UINT_PTR>(Menu::File_Extract): {
            wchar_t tempPathToExtract[MAX_PATH] = { 0 };
            BROWSEINFOW bi = { 0 };
            bi.hwndOwner = hwnd;
            bi.lpszTitle = L"Выберите папку";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

            PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
            if (pidl) {
                SHGetPathFromIDListW(pidl, tempPathToExtract);
                CoTaskMemFree(pidl);

                int curTab = TabCtrl_GetCurSel(wls->hTabControl);
                auto curFiles = GetSelectedItems(wls->vectorFilesInfo[curTab].hListView);
                for (auto i : curFiles) {
                    wchar_t* tempForExtract = new wchar_t[MAX_PATH];
                    ListView_GetItemText(wls->vectorFilesInfo[curTab].hListView, i, 0, tempForExtract, MAX_PATH);
                    
                    std::wstring finalPath(tempPathToExtract);
                    finalPath += L"/";
                    finalPath += tempForExtract;

                    char* fileNameForExtract = new char[MAX_PATH];
                    int length = WideCharToMultiByte(CP_ACP, 0, tempForExtract, -1, fileNameForExtract, MAX_PATH, NULL, NULL);
                    
                    wls->operationForFileType[wls->vectorFilesInfo[curTab].extension].ExportFile(wls->vectorFilesInfo[curTab].token,
                        fileNameForExtract, finalPath.c_str());

                    delete[] fileNameForExtract;
                    delete[] tempForExtract;
                }
                
            }
            break;
        }
        case static_cast<UINT_PTR>(Menu::About): {
            MessageBoxW(hwnd, L"No-ZIP\n\nТестовая сборка архиватора\n\nВерсия 0.1", L"Что это?", MB_ICONQUESTION);
            break;
        }
        case static_cast<UINT_PTR>(Menu::File_Exit): {
            PostQuitMessage(0);
            break;
        }
        case static_cast<UINT_PTR>(Menu::Parameters_Modules): {
            wls->modulesListWindow->Show();
            break;
        }
        }
        break;
    }
    case WM_NOTIFY: {
        NMHDR* hdr = reinterpret_cast<NMHDR*>(lparam);
        switch (hdr->code) {
        case TCN_SELCHANGE: {
            for (auto& i : wls->vectorFilesInfo)
                ShowWindow(i.hListView, SW_HIDE);
            
            ShowWindow(wls->vectorFilesInfo[TabCtrl_GetCurSel(wls->hTabControl)].hListView, SW_SHOWNORMAL);
            break;
        }
        case LVN_ITEMCHANGED: {
            EnableMenuItem(wls->hMenuFile, static_cast<UINT>(Menu::File_Extract), ListView_GetNextItem(wls->vectorFilesInfo[TabCtrl_GetCurSel(wls->hTabControl)].hListView, -1, LVNI_SELECTED) == -1 ? MF_DISABLED : MF_ENABLED);
            break;
        }
        }
        break;
    }
    //// TODO:
    //// Для реализации кнопки закрытия 
    //case WM_DRAWITEM: {
    //    DRAWITEMSTRUCT* lpDrawItemStruct = reinterpret_cast<DRAWITEMSTRUCT*>(lparam);

    //    wchar_t buf[MAX_PATH] = { 0 };
    //    TCITEMW item;
    //    item.mask = TCIF_TEXT;
    //    item.cchTextMax = MAX_PATH;
    //    item.pszText = buf;
    //    TabCtrl_GetItem(wls->hTabControl, lpDrawItemStruct->itemID, &item);
    //    DrawTextW(lpDrawItemStruct->hDC, buf, wcslen(buf), &lpDrawItemStruct->rcItem, DT_LEFT);

    //    SelectObject(lpDrawItemStruct->hDC, wls->hFontForCloseTab);
    //    DrawTextW(lpDrawItemStruct->hDC, L"× ", 2, &lpDrawItemStruct->rcItem, DT_RIGHT);
    //    break;
    //}
    case WM_DESTROY: PostQuitMessage(0);
    default: return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    return 0;
}

int main() {

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    SetProcessDPIAware();

    HMONITOR hPrimaryMonitor = MonitorFromWindow(NULL, MONITOR_DEFAULTTOPRIMARY);
    currentDPI = GetMonitorDPI(hPrimaryMonitor);

    Registry reg;
    reg.Open(HKEY_CURRENT_USER, L"Software\\No-ZIP");

    auto keys = reg.GetSubKeys();
    for (auto& i : keys) {
        settingsTable[i] = reg.GetKeyDefaultValue(i.c_str());
    }

    std::unordered_map<std::wstring, loadingLibrary> operationForFileType;
    for (auto& i : settingsTable) {
        loadingLibrary temp;
        temp.hInstance = LoadLibraryW(i.second.c_str());

        if (!temp.hInstance) {
            DisplayError();
            return 1;
        }

        temp.OpenArchive = GetFunction<OpenArchiveType>(temp.hInstance, "OpenArchive");
        temp.GetFileCount = GetFunction<GetFileCountType>(temp.hInstance, "GetFileCount");
        temp.GetListOfFiles = GetFunction<GetListOfFilesType>(temp.hInstance, "GetListOfFiles");
        temp.ExportFile = GetFunction<ExportFileType>(temp.hInstance, "ExportFile");
        temp.CloseFile = GetFunction<CloseFileType>(temp.hInstance, "CloseFile");

        if (!temp.OpenArchive || !temp.GetFileCount || !temp.GetListOfFiles || !temp.ExportFile || !temp.CloseFile) {
            DisplayError();
            FreeLibrary(temp.hInstance);
            return 1;
        }

        operationForFileType[i.first] = temp;
    }

    ModulesListWindow mlw(settingsTable, reg, currentDPI);

    WindowLocalStruct wls = { &mlw, operationForFileType };

    CreateClassExW((LPWSTR)L"No-ZIP", (HBRUSH)GetStockObject(WHITE_BRUSH), IDC_ARROW, DefZIPWindowProc, sizeof(WindowLocalStruct*));

    HWND hWindow = CreateWindowExW(WS_EX_ACCEPTFILES, L"No-ZIP", L"No-Zip", WS_OVERLAPPEDWINDOW, ScaleForDPI(10, currentDPI), ScaleForDPI(10, currentDPI), ScaleForDPI(640, currentDPI), ScaleForDPI(480, currentDPI), 0, 0, 0, 0);
    
    wls.hFontForCloseTab = CreateFontW(ScaleForDPI(20, currentDPI), 0, 0, 0, 400, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    wls.hTabControl = CreateWindowExW(NULL, L"SysTabControl32", L"Tabs", WS_CHILD | WS_VISIBLE | TCS_FOCUSNEVER /*| TCS_OWNERDRAWFIXED*/, 0, 0, ScaleForDPI(620, currentDPI), ScaleForDPI(30, currentDPI), hWindow, 0, 0, 0);

    HMENU hMenu = CreateMenu();
    wls.hMenuFile = CreatePopupMenu();
    HMENU hMenuEdit = CreatePopupMenu();
    HMENU hMenuSettings = CreatePopupMenu();
    HMENU hMenuAbout = CreatePopupMenu();

    AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)wls.hMenuFile, L"Файл");
    AppendMenuW(wls.hMenuFile, MF_STRING, static_cast<UINT_PTR>(Menu::File_Open), L"Открыть");
    AppendMenuW(wls.hMenuFile, MF_STRING | MF_DISABLED, static_cast<UINT_PTR>(Menu::File_Extract), L"Извлечь");
    AppendMenuW(wls.hMenuFile, MF_SEPARATOR, 0, L"");
    AppendMenuW(wls.hMenuFile, MF_STRING, static_cast<UINT_PTR>(Menu::File_Exit), L"Выход");

    AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hMenuSettings, L"Параметры");
    AppendMenuW(hMenuSettings, MF_STRING, static_cast<UINT_PTR>(Menu::Parameters_Modules), L"Модули...");

    AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hMenuAbout, L"О программе");
    AppendMenuW(hMenuAbout, MF_STRING, static_cast<UINT_PTR>(Menu::About), L"Что это?");

    SetMenu(hWindow, hMenu);

    SetWindowLongPtrW(hWindow, GWLP_USERDATA, (LONG_PTR) &wls);

    ShowWindow(hWindow, SW_SHOWNORMAL);

    MSG message;
    while (GetMessageW(&message, 0, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    
	return 0;
}