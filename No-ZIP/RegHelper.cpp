#include "RegHelper.h"

LRESULT Registry::Open(HKEY hKey, LPCWSTR lpSubKey) {
	LSTATUS result = RegOpenKeyW(hKey, lpSubKey, &this->hKey);
	if (ERROR_FILE_NOT_FOUND == result) {
		DWORD disposition;
		result = RegCreateKeyExW(hKey, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &this->hKey, &disposition);
	}
	return result;
}

std::vector<std::wstring> Registry::GetSubKeys() {
    wchar_t keyName[256];
    DWORD keyNameSize = sizeof(keyName) / sizeof(keyName[0]);
    DWORD index = 0;
    LRESULT result;

    std::vector<std::wstring> subKeys;

    while (true) {
        memset(keyName, 0, sizeof(keyName));
        keyNameSize = sizeof(keyName) / sizeof(keyName[0]);

        result = RegEnumKeyExW(this->hKey, index, keyName, &keyNameSize, NULL, NULL, NULL, NULL);

        if (ERROR_NO_MORE_ITEMS == result) break;

        if (ERROR_SUCCESS == result) {
            subKeys.emplace_back(keyName);
            index++;
        }
    }

    return subKeys;
}

std::wstring Registry::GetKeyDefaultValue(LPCWSTR lpSubKey) {
    DWORD dataType;
    DWORD dataSize = 0;

    LONG result = RegGetValueW(this->hKey, lpSubKey, L"", RRF_RT_REG_SZ, &dataType, NULL, &dataSize);
    if (result != ERROR_SUCCESS) return L"";

    if (dataType != REG_SZ && dataType != REG_EXPAND_SZ) return L"";

    std::vector<wchar_t> buffer(dataSize / sizeof(wchar_t) + 1);

    result = RegGetValueW(this->hKey, lpSubKey, L"", RRF_RT_REG_SZ, &dataType, buffer.data(), &dataSize);
    if (result != ERROR_SUCCESS) return L"";

    buffer.back() = L'\0';
    return std::wstring(buffer.data());
}

bool Registry::SetKeyDefaultValue(LPCWSTR lpSubKey, const std::wstring& value) {
    DWORD dataSize = (value.length() + 1) * sizeof(wchar_t);

    return RegSetKeyValueW(this->hKey, lpSubKey, L"", REG_SZ, value.c_str(), dataSize) == ERROR_SUCCESS;
}

LSTATUS Registry::DeleteKey(LPCWSTR lpSubKey) {
    return RegDeleteKeyW(this->hKey, lpSubKey);
}

Registry::~Registry() {
	RegCloseKey(this->hKey);
}