// http.h
#pragma once
#include <aclapi.h>
#include <commctrl.h>
#include <winhttp.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <locale>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <windows.h>
#pragma comment(lib, "winhttp.lib")

bool SendHttpRequest(const wchar_t* hostname, const wchar_t* path, const wchar_t* method, const wchar_t* headers, const char* body, size_t bodyLen, void* buffer, size_t maxlen, bool useSSL = true);

std::string W2U(const std::wstring& ws);
std::wstring U2W(const std::string& s);
std::string url_encode(const std::string& s);
std::string json_escape(const std::string& s);