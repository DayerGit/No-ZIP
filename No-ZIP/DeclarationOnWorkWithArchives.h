#pragma once
#include "DeclarationOfFile.h"
#include <functional>
#include <Windows.h>

using FileToken = unsigned long long;

using OpenArchiveType = FileToken(__stdcall*)(const wchar_t* path);
using GetFileCountType = size_t(__stdcall*)(FileToken _token);
using GetListOfFilesType = FileSpecification*(__stdcall*)(FileToken _token);
using ExportFileType = bool(__stdcall*)(FileToken _token, const char* filename, const wchar_t* path);
using CloseFileType = void(__stdcall*)(FileToken _token);

struct loadingLibrary {
	HINSTANCE hInstance;
	std::function <FileToken(const wchar_t* path)> OpenArchive;
	std::function <size_t(FileToken _token)> GetFileCount;
	std::function <FileSpecification* (FileToken _token)> GetListOfFiles;
	std::function <bool(FileToken _token, const char* filename, const wchar_t* path)> ExportFile;
	std::function <void(FileToken _token)> CloseFile;
};

template<typename T>
T GetFunction(HINSTANCE hInstance, const char* functionName) {
	return reinterpret_cast<T>(GetProcAddress(hInstance, functionName));
}