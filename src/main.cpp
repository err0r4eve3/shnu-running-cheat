// main.cpp

#include "../include/main.h"
#include "../include/http.h"

BYTE pattern[] = { 0x3F, 0x7B, 0x22, 0x64, 0x61, 0x74, 0x61, 0x22, 0x3A, 0x22 };  // ?{"data":
char userId[33] = { 0 };

static char* fetchUserId() { // 原登录过程使用微信内部标识符openId 只能通过内部api获取 所以无法模拟登录过程 选择读内存获取userId标识符
    HWND hwnd = FindWindowW(NULL, L"体锻打卡");
    if (!hwnd) return nullptr;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) return nullptr;
    uintptr_t addr = ScanProcessMemory(hProcess, pattern, sizeof(pattern));
    if (!addr) return nullptr;
    ReadProcessMemory(hProcess, (const void*)(addr + sizeof(pattern)), userId, 32, NULL);
    userId[32] = '\0';
    CloseHandle(hProcess);
    return userId;
}

static std::string ExtractRunningRecord(const char* jsonResponse) { // 暴力解析recordId
    if (!jsonResponse) return {};
    std::string s(jsonResponse);

    const std::string key = "\"runningRecord\":\"";
    size_t p = s.find(key);
    if (p == std::string::npos) return {};
    p += key.size();

    size_t q = s.find('"', p);
    if (q == std::string::npos) return {};

    std::string rid = s.substr(p, q - p);

    return rid;
}

static bool CallOKAPI(const OkReq& req, char* response, size_t responseSize) {
    std::string body = req.BuildBody();
    std::wstring headers = req.Headers();
    return SendHttpRequest(
        L"cpapp.1lesson.cn",
        L"/api/route/selectStudentSignIn",
        L"POST",
        headers.c_str(),
        body.c_str(),
        body.length(),
        response,
        responseSize,
        true
    );
}

static bool CallStartRunAPI(const StartRunReq& req, char* response, size_t responseSize) {
    std::string body = req.BuildBody();
    std::wstring headers = req.Headers();
    return SendHttpRequest(
        L"cpapp.1lesson.cn",
        L"/api/route/insertStartRunning",
        L"POST",
        headers.c_str(),
        body.c_str(),
        body.length(),
        response,
        responseSize,
        true
    );
}

static bool CallEndRunAPI(const EndRunReq& req, char* response, size_t responseSize) {
    std::string body = req.BuildBody();
    std::wstring headers = req.Headers();
    return SendHttpRequest(
        L"cpapp.1lesson.cn",
        L"/api/route/insertFinishRunning",
        L"POST",
        headers.c_str(),
        body.c_str(),
        body.length(),
        response,
        responseSize,
        true
    );
}

int main()
{
    char* userId = fetchUserId();
    printf("userId=%s\n", userId);
    char response[4096] = { 0 };
    StartRunReq startRunReq;
    startRunReq.userId = userId;
    CallStartRunAPI(startRunReq, response, sizeof(response));
    std::string recordId = ExtractRunningRecord(response);

    OkReq okReq;

    if (recordId.empty()) {
        fprintf(stderr, "解析 runningRecord 失败\n");
        return 1;
    }

    for (int i = 0; i < 4; ++i) {
        okReq.userId = userId;
        okReq.recordId = recordId;
        okReq.posLongitude = Coords[i][0];
        okReq.posLatitude = Coords[i][1];
        Sleep(1000);
        CallOKAPI(okReq, response, sizeof(response));
    }
    EndRunReq endRunReq;
    endRunReq.userId = userId;
    endRunReq.runningRecordId = recordId;
    CallEndRunAPI(endRunReq, response, sizeof(response));
    std::string endBody = endRunReq.BuildBody();

    return 0;
}