#pragma once
#include <Windows.h>
#include <vector>
#include <string>

class Registry {
public:
	Registry() = default;

	LRESULT Open(HKEY hKey, LPCWSTR lpSubKey);
	std::vector<std::wstring> GetSubKeys();
	std::wstring GetKeyDefaultValue(LPCWSTR lpSubKey);
	bool SetKeyDefaultValue(LPCWSTR lpSubKey, const std::wstring& value);
	LSTATUS DeleteKey(LPCWSTR lpSubKey);

	~Registry();

private:
	HKEY hKey;
};
