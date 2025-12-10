// main.cpp

#include "../include/main.h"
#include "../include/http.h"

// 显式声明，确保使用带起始地址的重载
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
    uintptr_t addr = ScanProcessMemory(hProcess, pattern, sizeof(pattern), 0);
    ULONGLONG t0 = GetTickCount64();
    while (addr) {
        char buf[64] = { 0 };
        ReadProcessMemory(hProcess, (const void*)(addr + sizeof(pattern)), buf, sizeof(buf), NULL);

        // 打印命中内容（原样打印前 48 字节，便于调试）
        char preview[49] = { 0 };
        memcpy(preview, buf, 48);
        printf("[扫描] addr=0x%p text=%s\n", (void*)addr, preview);
        fflush(stdout);

        bool ok = true;
        for (int i = 0; i < 32; ++i) {
            if (!std::isxdigit((unsigned char)buf[i])) { ok = false; break; }
        }
        if (ok && !std::isxdigit((unsigned char)buf[32]) && buf[0] != '\0') {
            strncpy_s(userId, buf, 32);
            userId[32] = '\0';
            break;
        }
        addr = ScanProcessMemory(hProcess, pattern, sizeof(pattern), addr + 1);
        if (GetTickCount64() - t0 > 5000) { // 超过5秒认为扫描超时
            fprintf(stderr, "[错误] 扫描 userId 超时，请确认目标窗口/进程可读\n");
            break;
        }
    }

    if (userId[0] == '\0') {
        CloseHandle(hProcess);
        return nullptr;
    }
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
        L"/api/route/selectStudentRunningRecord",
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

// 对给定 polyline 施加轻微噪声并平滑，避免笔直痕迹（扰动量级 ~2m）
static std::vector<Point> AddNoiseAndSmooth(const std::vector<Point>& src) {
    if (src.empty()) return {};
    std::mt19937 rng((uint32_t)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::normal_distribution<double> noise_m(0.0, 2.0); // 均值0，标准差2米

    std::vector<Point> noisy;
    noisy.reserve(src.size());
    for (auto& p : src) {
        // 米 -> 经纬度近似换算
        constexpr double PI = 3.14159265358979323846;
        double lat_per_m = 1.0 / 111111.0;
        double lon_per_m = 1.0 / (111111.0 * std::cos(p.latitude * PI / 180.0));
        double dn = noise_m(rng);
        double de = noise_m(rng);
        noisy.push_back(Point{
            p.latitude + dn * lat_per_m,
            p.longitude + de * lon_per_m
            });
    }

    // 简单滑动平均平滑
    std::vector<Point> smooth(noisy.size());
    for (size_t i = 0; i < noisy.size(); ++i) {
        double lat = 0.0, lon = 0.0;
        int cnt = 0;
        for (int k = -1; k <= 1; ++k) {
            int idx = static_cast<int>(i) + k;
            if (idx >= 0 && idx < (int)noisy.size()) {
                lat += noisy[idx].latitude;
                lon += noisy[idx].longitude;
                ++cnt;
            }
        }
        smooth[i].latitude = lat / cnt;
        smooth[i].longitude = lon / cnt;
    }
    return smooth;
}

// 将关键点锚定回原始坐标，确保轨迹必经预设点
static std::vector<Point> AnchorRoute(const std::vector<Point>& original, const std::vector<Point>& noisy, const std::vector<size_t>& anchors) {
    std::vector<Point> out = noisy;
    for (size_t idx : anchors) {
        if (idx < out.size() && idx < original.size()) {
            out[idx] = original[idx];
        }
    }
    return out;
}

static double HaversineMeters(const Point& a, const Point& b) {
    constexpr double R = 6378137.0;
    constexpr double PI = 3.14159265358979323846;
    double lat1 = a.latitude * PI / 180.0;
    double lat2 = b.latitude * PI / 180.0;
    double dlat = lat2 - lat1;
    double dlon = (b.longitude - a.longitude) * PI / 180.0;
    double s = std::sin(dlat / 2) * std::sin(dlat / 2) +
        std::cos(lat1) * std::cos(lat2) * std::sin(dlon / 2) * std::sin(dlon / 2);
    double c = 2 * std::atan2(std::sqrt(s), std::sqrt(1 - s));
    return R * c;
}

static std::vector<double> BuildCumulativeDistances(const std::vector<Point>& path) {
    std::vector<double> cum;
    cum.reserve(path.size());
    double total = 0.0;
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) total += HaversineMeters(path[i - 1], path[i]);
        cum.push_back(total);
    }
    return cum;
}

static Point SampleAtTime(const std::vector<Point>& path, const std::vector<double>& cum, uint32_t totalSeconds, uint32_t t) {
    if (path.empty()) return Point{};
    if (t >= totalSeconds || cum.back() == 0) return path.back();
    double target = cum.back() * (static_cast<double>(t) / static_cast<double>(totalSeconds));
    auto it = std::lower_bound(cum.begin(), cum.end(), target);
    size_t idx = std::distance(cum.begin(), it);
    if (idx == 0) return path.front();
    if (idx >= path.size()) return path.back();
    double d0 = cum[idx - 1];
    double d1 = cum[idx];
    double denom = (d1 - d0);
    if (denom < 1e-6) denom = 1e-6;
    double ratio = (target - d0) / denom;
    Point res;
    res.latitude = path[idx - 1].latitude + (path[idx].latitude - path[idx - 1].latitude) * ratio;
    res.longitude = path[idx - 1].longitude + (path[idx].longitude - path[idx - 1].longitude) * ratio;
    return res;
}

int main()
{
    char manualId[128] = { 0 };
    printf("如需手动输入 userId，请键入后回车（直接回车则自动扫描）：");
    if (fgets(manualId, sizeof(manualId), stdin)) {
        // 去掉行尾换行
        size_t len = strlen(manualId);
        if (len > 0 && (manualId[len - 1] == '\n' || manualId[len - 1] == '\r')) {
            manualId[len - 1] = '\0';
            if (len > 1 && manualId[len - 2] == '\r') {
                manualId[len - 2] = '\0';
            }
        }
    } else {
        manualId[0] = '\0';
    }

    char* userId = nullptr;
    if (manualId[0] != '\0') {
        userId = manualId;
    } else {
        userId = fetchUserId();
    }

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

    // 固定路线与打卡点（总里程约 2.1km）
    static const std::vector<Point> route = {
        {30.837721, 121.518181}, // 打卡点1
        {30.837661, 121.518162},
        {30.837300, 121.518674},
        {30.836944, 121.519175},
        {30.836866, 121.519121},
        {30.836707, 121.519134},
        {30.836076, 121.519432},
        {30.835999, 121.519527},
        {30.835759, 121.519739},
        {30.835666, 121.519946},
        {30.835627, 121.520077},
        {30.835623, 121.520208},
        {30.835708, 121.520488},
        {30.835596, 121.520605},
        {30.834883, 121.520928}, // 打卡点2
        {30.835596, 121.520605},
        {30.835708, 121.520488},
        {30.835623, 121.520208},
        {30.835627, 121.520077},
        {30.835666, 121.519946},
        {30.835759, 121.519739},
        {30.835999, 121.519527},
        {30.835872, 121.519261}, // 打卡点3
        {30.835419, 121.517462},
        {30.835306, 121.517498}, // 打卡点4
        {30.836003, 121.519504},
        {30.835306, 121.517498},
        {30.836003, 121.519504},
        {30.835306, 121.517498},
        {30.836003, 121.519504},
        {30.835306, 121.517498}
    };
    // 生成噪声平滑后的路线，并锚定全部原始点，确保必经预设点
    std::vector<size_t> anchorIdx(route.size());
    for (size_t i = 0; i < route.size(); ++i) anchorIdx[i] = i;
    const std::vector<Point> noisyRouteBase = AddNoiseAndSmooth(route);
    const std::vector<Point> noisyRoute = AnchorRoute(route, noisyRouteBase, anchorIdx);
    const std::vector<double> cumDist = BuildCumulativeDistances(noisyRoute);
    const double pathTotalKm = cumDist.empty() ? 0.0 : cumDist.back() / 1000.0;
    std::vector<Point> markers = { noisyRoute[0], noisyRoute[14], noisyRoute[22], noisyRoute[24] };

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

    // 时间轴模拟：每秒倒计时，OK 按计划时间，save 每 5 秒
    OkReq okReq;
    okReq.userId = userId;
    okReq.recordId = recordId;

    uint64_t startMs = NowMsEpoch();
    uint64_t endMsPlan = startMs + 25ull * 60ull * 1000ull;
    double targetMileage = pathTotalKm > 0 ? pathTotalKm : 2.0; // km

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
            Point p = SampleAtTime(noisyRoute, cumDist, totalSeconds, okSchedule[okIdx]);
            okReq.posLongitude = p.longitude;
            okReq.posLatitude = p.latitude;
            ZeroMemory(response, sizeof(response));
            bool okSuccess = CallOKAPI(okReq, response, sizeof(response));
            printf("[调试] 第%zu次 OK 上报(%s) 响应: %s\n", okIdx + 1, okSuccess ? "成功" : "失败", response);
            ++okIdx;
        }

        if (elapsed > 0 && (elapsed == nextSave || elapsed == totalSeconds)) {
            double denom = (totalSeconds == 0) ? 1.0 : static_cast<double>(totalSeconds);
            double distMeters = cumDist.empty() ? 0.0 : cumDist.back() * (static_cast<double>(elapsed) / denom);
            saveReq.mileage = distMeters / 1000.0;
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
    endRunReq.mileage = pathTotalKm > 0 ? pathTotalKm : targetMileage;
    endRunReq.totalTime = static_cast<double>(totalSeconds) / 60.0; // min
    endRunReq.data = noisyRoute;

    ZeroMemory(response, sizeof(response));
    bool endOk = CallEndRunAPI(endRunReq, response, sizeof(response));
        printf("\n[调试] EndRun 请求体: %s\n", endRunReq.BuildBody().c_str());
    printf("[调试] EndRun 返回(%s): %s\n", endOk ? "成功" : "失败", response);

    return 0;
}