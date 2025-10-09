// main.cpp

#include "../include/main.h"
#include "../include/http.h"

BYTE pattern[] = { 0x75, 0x73, 0x65, 0x72, 0x49, 0x64, 0x3D };  // userId=
char userId[33] = { 0 };

// 原登录过程使用微信内部标识符openId 只能通过内部api获取 所以无法模拟登录过程 选择读内存获取userId标识符 扫描pattern"userId=xxxxxxxx"
static char* fetchUserId() { 
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

static std::string ExtractRunningRecord(const char* jsonResponse) {
    if (!jsonResponse) return {};
    try {
        auto j = json::parse(jsonResponse);
        if (j.contains("data") && j["data"].contains("runningRecord") && j["data"]["runningRecord"].is_string()) {
            return j["data"]["runningRecord"].get<std::string>();
        }
        if (j.contains("data") && j["data"].is_string()) {
            return j["data"].get<std::string>();
        }
    }
    catch (...) {
    }
    return {};
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

static bool CallSelectStudentRunningRecord(const SelectStudentRunningRecordReq& req, char* response, size_t responseSize) {
    std::string body = req.BuildBody();
    std::wstring headers = StudentRunningRecordHeaders();
    return SendHttpRequest(
        L"cpapp.1lesson.cn",
        L"/api/student/selectStudentRunningRecord",
        L"POST",
        headers.c_str(),
        body.c_str(),
        body.length(),
        response,
        responseSize,
        true
    );
}

static bool CallProjectMessage(const ProjectMessageReq& req, char* response, size_t responseSize) {
    std::wstring headers = req.Headers();
    std::string path = "/api/project/selectProjectMessage?userId=" + req.userId;
    return SendHttpRequest(
        L"cpapp.1lesson.cn",
        U2W(path).c_str(),
        L"GET",
        headers.c_str(),
        nullptr,
        0,
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

static bool CallRandomLineAPI(const RandomLineReq& req, char* response, size_t responseSize) {
    std::string body = req.BuildBody();
    std::wstring headers = req.Headers();
    return SendHttpRequest(
        L"cpapp.1lesson.cn",
        L"/api/route/selectRandomRoute",
        L"POST",
        headers.c_str(),
        body.c_str(),
        body.length(),
        response,
        responseSize,
        true
    );
}

static bool CallRandomPointAPI(const RandomPointReq& req, char* response, size_t responseSize) {
    std::string body = req.BuildBody();
    std::wstring headers = req.Headers();
    return SendHttpRequest(
        L"cpapp.1lesson.cn",
        L"/api/route/selectRoute",
        L"POST",
        headers.c_str(),
        body.c_str(),
        body.length(),
        response,
        responseSize,
        true
    );
}

struct SaveRunningDataReq {
    std::string userId;
    std::string runningRecordId;
    double mileage = 0;
    std::string speedAllocation = "10,00";
    double totalTime = 0.0; // min
    unsigned int timeLeft = 120;
    uint64_t nowtime = 0;
    uint64_t nowtimer = 0;

    std::string BuildBody() const {
        json j;
        j["userId"] = userId;
        j["runningRecordId"] = runningRecordId;
        j["mileage"] = std::stod(dtoa_fixed(mileage, 2));
        j["speedAllocation"] = speedAllocation;
        j["totalTime"] = std::stod(dtoa_fixed(totalTime, 2)); // min
        j["timeLeft"] = timeLeft;
        j["nowtime"] = nowtime;
        j["nowtimer"] = nowtimer;
        return j.dump();
    }

    std::wstring Headers() const {
        return MiniProgramHeadersJson();
    }
};

static bool CallSaveRunningDataAPI(const SaveRunningDataReq& req, char* response, size_t responseSize) {
    std::string body = req.BuildBody();
    std::wstring headers = req.Headers();
    return SendHttpRequest(
        L"cpapp.1lesson.cn",
        L"/api/route/saveRunningData",
        L"POST",
        headers.c_str(),
        body.c_str(),
        body.length(),
        response,
        responseSize,
        true
    );
}

static std::vector<Point> ExtractPoints(const char* jsonResponse) {
    std::vector<Point> pts;
    if (!jsonResponse) return pts;
    std::string s(jsonResponse);
    size_t pos = 0;
    const std::string keyLat = "\"latitude\":";
    const std::string keyLon = "\"longitude\":";
    while (true) {
        size_t pLat = s.find(keyLat, pos);
        if (pLat == std::string::npos) break;
        pLat += keyLat.size();
        size_t pLatEnd = s.find_first_of(",}", pLat);
        if (pLatEnd == std::string::npos) break;
        size_t pLon = s.find(keyLon, pLatEnd);
        if (pLon == std::string::npos) break;
        pLon += keyLon.size();
        size_t pLonEnd = s.find_first_of(",}", pLon);
        if (pLonEnd == std::string::npos) break;
        try {
            double lat = std::stod(s.substr(pLat, pLatEnd - pLat));
            double lon = std::stod(s.substr(pLon, pLonEnd - pLon));
            pts.push_back(Point{ lat, lon });
        }
        catch (...) {
        }
        pos = pLonEnd;
        if (pts.size() >= 4) break; // 小程序固定4个点，够用即可
    }
    return pts;
}

static std::string FormatTime(uint32_t seconds) {
    uint32_t h = seconds / 3600;
    uint32_t m = (seconds % 3600) / 60;
    uint32_t s = seconds % 60;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02u:%02u:%02u", h, m, s);
    return std::string(buf);
}

static uint64_t NowMsEpoch() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

static std::vector<uint32_t> BuildOkSchedule(uint32_t totalSeconds) {
    std::vector<uint32_t> sched;
    uint32_t last;
    if (totalSeconds >= 120) {
        last = totalSeconds > 45 ? totalSeconds - 45 : totalSeconds;
    }
    else if (totalSeconds >= 60) {
        last = totalSeconds > 30 ? totalSeconds - 30 : totalSeconds;
    }
    else {
        last = totalSeconds;
    }
    for (int i = 0; i < 4; ++i) {
        uint32_t t = static_cast<uint32_t>(std::round((double)last * i / 3.0));
        if (t > totalSeconds) t = totalSeconds;
        sched.push_back(t);
    }
    // 确保非递减
    for (size_t i = 1; i < sched.size(); ++i) {
        if (sched[i] < sched[i - 1]) sched[i] = sched[i - 1];
    }
    return sched;
}

static std::vector<Point> BuildPolyline(const std::vector<Point>& markers, uint32_t totalSeconds) {
    std::vector<Point> poly;
    if (markers.empty()) return poly;
    // 期望点数：心跳次数+冗余，至少 40
    uint32_t desired = std::max<uint32_t>(static_cast<uint32_t>(totalSeconds / 5) + 1, 40);
    size_t segments = markers.size() > 1 ? markers.size() - 1 : 1;
    uint32_t stepsPerSeg = static_cast<uint32_t>((desired + segments - 1) / segments);
    stepsPerSeg = std::max<uint32_t>(stepsPerSeg, 10);

    for (size_t i = 0; i + 1 < markers.size(); ++i) {
        const auto& a = markers[i];
        const auto& b = markers[i + 1];
        poly.push_back(a);
        for (uint32_t s = 1; s < stepsPerSeg; ++s) {
            double t = static_cast<double>(s) / static_cast<double>(stepsPerSeg);
            double lat = a.latitude + (b.latitude - a.latitude) * t;
            double lon = a.longitude + (b.longitude - a.longitude) * t;
            poly.push_back(Point{ lat, lon });
        }
    }
    poly.push_back(markers.back());
    return poly;
}

int main()
{
    char* userId = fetchUserId();
    if (!userId || !*userId) {
        fprintf(stderr, "[错误] 获取 userId 失败，请确认小程序窗口是否打开\n");
        return 1;
    }
    printf("[信息] userId=%s\n", userId);
    char response[4096] = { 0 };

    // 询问跑步时长（分钟，0-25）
    printf("请输入跑步总时长（分钟，0-25）：");
    int runMinutes = 0;
    if (scanf_s("%d", &runMinutes) != 1 || runMinutes < 0) runMinutes = 0;
    if (runMinutes > 25) runMinutes = 25;
    const uint32_t totalSeconds = static_cast<uint32_t>(runMinutes * 60);

    // 
    std::vector<Point> markers = {
        {30.837721, 121.518181},
        {30.834883, 121.520928},
        {30.835872, 121.519261},
        {30.835306, 121.517498}
    };

    // 预计算四次打卡时间点：均匀分布，最后一次尽量在结束前 30~60 秒
    std::vector<uint32_t> okSchedule = BuildOkSchedule(totalSeconds);

    // StartRun
    StartRunReq startRunReq;
    startRunReq.userId = userId;
    ZeroMemory(response, sizeof(response));
    bool startOk = CallStartRunAPI(startRunReq, response, sizeof(response));
    printf("[调试] StartRun 请求体: %s\n", startRunReq.BuildBody().c_str());
    printf("[调试] StartRun 返回(%s): %s\n", startOk ? "成功" : "失败", response);
    std::string recordId = ExtractRunningRecord(response);

    if (recordId.empty()) {
        fprintf(stderr, "[错误] 解析 runningRecord 失败，响应内容: %s\n", response);
        return 1;
    }
    printf("[信息] 解析到 runningRecord=%s\n", recordId.c_str());

    // 时间轴模拟：每秒倒计时，OK 间隔 5 分钟，save 每 5 秒
    OkReq okReq;
    okReq.userId = userId;
    okReq.recordId = recordId;

    uint64_t startMs = NowMsEpoch();
    uint64_t endMsPlan = startMs + 25ull * 60ull * 1000ull;
    double targetMileage = 2.0; // km

    SaveRunningDataReq saveReq;
    saveReq.userId = userId;
    saveReq.runningRecordId = recordId;
    saveReq.speedAllocation = "10,00";
    saveReq.timeLeft = totalSeconds > 25 * 60 ? 0 : (25 * 60 - totalSeconds);
    saveReq.nowtime = startMs;
    saveReq.nowtimer = endMsPlan;

    uint32_t elapsed = 0;
    uint32_t nextSave = 5;
    size_t okIdx = 0;

    while (elapsed <= totalSeconds) {
        if (okIdx < okSchedule.size() && okSchedule[okIdx] > elapsed) {
            uint32_t remain = okSchedule[okIdx] > elapsed ? okSchedule[okIdx] - elapsed : 0;
            uint32_t mm = remain / 60;
            uint32_t ss = remain % 60;
            printf("\r[倒计时] 距下一次打卡剩余 %02u:%02u   ", mm, ss);
            fflush(stdout);
        } else if (okIdx >= okSchedule.size()) {
            printf("\r[倒计时] 所有打卡已完成   ");
            fflush(stdout);
        }

        if (okIdx < okSchedule.size() && elapsed == okSchedule[okIdx]) {
            size_t markerIdx = okIdx % markers.size();
            okReq.posLongitude = markers[markerIdx].longitude;
            okReq.posLatitude = markers[markerIdx].latitude;
            ZeroMemory(response, sizeof(response));
            bool okSuccess = CallOKAPI(okReq, response, sizeof(response));
            printf("[调试] 第%zu次 OK 上报(%s) 响应: %s\n", okIdx + 1, okSuccess ? "成功" : "失败", response);
            ++okIdx;
        }

        if (elapsed > 0 && (elapsed == nextSave || elapsed == totalSeconds)) {
            double ratio = totalSeconds == 0 ? 0.0 : static_cast<double>(elapsed) / static_cast<double>(totalSeconds);
            saveReq.mileage = targetMileage * ratio;
            saveReq.totalTime = static_cast<double>(elapsed) / 60.0; // 分钟
            saveReq.timeLeft = (elapsed >= 25 * 60) ? 0 : (25 * 60 - elapsed);
            ZeroMemory(response, sizeof(response));
            bool saveOk = CallSaveRunningDataAPI(saveReq, response, sizeof(response));
            printf("[调试] SaveRunningData t=%us 里程=%.2f totalTime=%.2f分钟 (%s): %s\n",
                elapsed, saveReq.mileage, saveReq.totalTime, saveOk ? "成功" : "失败", response);
            nextSave += 5;
        }

        if (elapsed == totalSeconds) break;
        Sleep(1000);
        ++elapsed;
    }

    EndRunReq endRunReq;
    endRunReq.userId = userId;
    endRunReq.runningRecordId = recordId;
    endRunReq.mileage = targetMileage;
    endRunReq.totalTime = static_cast<double>(totalSeconds) / 60.0; // min
    endRunReq.data = BuildPolyline(markers, totalSeconds);

    ZeroMemory(response, sizeof(response));
    bool endOk = CallEndRunAPI(endRunReq, response, sizeof(response));
        printf("\n[调试] EndRun 请求体: %s\n", endRunReq.BuildBody().c_str());
    printf("[调试] EndRun 返回(%s): %s\n", endOk ? "成功" : "失败", response);

    return 0;
}