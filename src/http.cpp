// http.cpp
#include "../include/http.h"

std::string W2U(const std::wstring& ws) {
    if (ws.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0,
        ws.data(), (int)ws.size(),
        nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0,
        ws.data(), (int)ws.size(),
        &s[0], n, nullptr, nullptr);
    return s;
}

std::wstring U2W(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0,
        s.data(), (int)s.size(),
        nullptr, 0);
    if (n <= 0) return {};
    std::wstring ws(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0,
        s.data(), (int)s.size(),
        &ws[0], n);
    return ws;
}

std::string url_encode(const std::string& s) {
    std::ostringstream oss;
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            oss << c;
        }
        else if (c == ' ') {
            oss << '+';
        }
        else {
            oss << '%' << std::uppercase << std::hex
                << std::setw(2) << std::setfill('0') << (int)c
                << std::nouppercase << std::dec;
        }
    }
    return oss.str();
}

std::string json_escape(const std::string& s) {
    std::string out; out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
        case '\"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b";  break;
        case '\f': out += "\\f";  break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            if (c < 0x20) {
                char buf[7];
                std::snprintf(buf, sizeof(buf), "\\u%04X", c);
                out += buf;
            }
            else {
                out += (char)c;
            }
        }
    }
    return out;
}

bool SendHttpRequest(const wchar_t* hostname, const wchar_t* path, const wchar_t* method, const wchar_t* headers, const char* body, size_t bodyLen, void* buffer, size_t maxlen, bool useSSL)
{
    HINTERNET hSession = 0, hConnect = 0, hRequest = 0;
    DWORD len, offset = 0;
    char* buf = (char*)buffer;
    bool success = false;

    hSession = WinHttpOpen(L"", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    if (hSession) {
        INTERNET_PORT port = useSSL ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
        hConnect = WinHttpConnect(hSession, hostname, port, 0);
    }

    if (hConnect) {
        DWORD flags = useSSL ? WINHTTP_FLAG_SECURE : 0;
        hRequest = WinHttpOpenRequest(hConnect, method, path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    }

    if (hRequest) {
        DWORD headersLen = headers ? wcslen(headers) : 0;
        if (WinHttpSendRequest(hRequest, headers, headersLen,
            (LPVOID)body, bodyLen, bodyLen, 0) &&
            WinHttpReceiveResponse(hRequest, NULL)) {
            while (WinHttpQueryDataAvailable(hRequest, &len) && len && offset < maxlen - 1) {
                DWORD bytesToRead = min(maxlen - offset - 1, len);
                if (WinHttpReadData(hRequest, buf + offset, bytesToRead, &len)) {
                    offset += len;
                }
                else {
                    break;
                }
            }
            buf[offset] = '\0';
            success = (offset > 0);
        }
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return success;
}