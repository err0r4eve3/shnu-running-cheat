#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <charconv>
#include <vector>
#include <chrono>
#include <cmath>
#include <random>
#include <algorithm>
#include <sstream>

#include "aobscan.h"
#include "http.h"
#include "json.hpp"

using json = nlohmann::json;

// 使用https://github.com/strengthen/AppletReverseTool逆向体锻打卡小程序 获取api接口以及相应请求数据结构

static inline std::string dtoa_fixed(double v, int precision = 14) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(precision);
    oss << v;
    return oss.str();
}

inline std::wstring MiniProgramHeadersForm() {
    return
        L"Content-Type: application/x-www-form-urlencoded\r\n"
        L"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/132.0.0.0 Safari/537.36 MicroMessenger/7.0.20.1781(0x6700143B) NetType/WIFI MiniProgramEnv/Windows WindowsWechat/WMPF WindowsWechat(0x63090a13) UnifiedPCWindowsWechat(0xf2541022) XWEB/16467\r\n"
        L"Referer: https://servicewechat.com/wx793a91cb16d89ad0/36/page-frame.html\r\n";
}

inline std::wstring MiniProgramHeadersJson() {
    return
        L"Content-Type: application/json; charset=UTF-8\r\n"
        L"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/132.0.0.0 Safari/537.36 MicroMessenger/7.0.20.1781(0x6700143B) NetType/WIFI MiniProgramEnv/Windows WindowsWechat/WMPF WindowsWechat(0x63090a13) UnifiedPCWindowsWechat(0xf2541022) XWEB/16467\r\n"
        L"Referer: https://servicewechat.com/wx793a91cb16d89ad0/36/page-frame.html\r\n";
}

inline std::wstring StudentRunningRecordHeaders() {
    // 完全按照用户提供的抓包头，逐行设置
    return
        L"user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/132.0.0.0 Safari/537.36 MicroMessenger/7.0.20.1781(0x6700143B) NetType/WIFI MiniProgramEnv/Windows WindowsWechat/WMPF WindowsWechat(0x63090a13) UnifiedPCWindowsWechat(0xf2541022) XWEB/16467\r\n"
        L"xweb_xhr: 1\r\n"
        L"content-type: application/x-www-form-urlencoded\r\n"
        L"accept: */*\r\n"
        L"sec-fetch-site: cross-site\r\n"
        L"sec-fetch-mode: cors\r\n"
        L"sec-fetch-dest: empty\r\n"
        L"referer: https://servicewechat.com/wx793a91cb16d89ad0/36/page-frame.html\r\n"
        L"accept-encoding: gzip, deflate, br\r\n"
        L"accept-language: zh-CN,zh;q=0.9\r\n"
        L"priority: u=1, i\r\n";
}

struct Point {
    double latitude{};
    double longitude{};
};

struct StartRunReq {
    std::string userId;
    std::string BuildBody() const {
        std::string body;
		body.reserve(32 + userId.size());
		body += "userId="; body += url_encode(userId);
        return body;
    }
    std::wstring Headers() const {
        return MiniProgramHeadersForm();
    }
};

struct SelectStudentRunningRecordReq {
    std::string userId;
    std::string BuildBody() const {
        std::string body;
        body.reserve(32 + userId.size());
        body += "userId="; body += url_encode(userId);
        return body;
    }
    std::wstring Headers() const { return MiniProgramHeadersForm(); }
};

struct ProjectMessageReq {
    std::string userId;
    std::wstring Headers() const { return MiniProgramHeadersForm(); }
};

struct OkReq {
    std::string userId;
    std::string recordId;
    double posLongitude{};
    double posLatitude{};

    std::string BuildBody() const {
        const std::string lon = dtoa_fixed(posLongitude, 14);
        const std::string lat = dtoa_fixed(posLatitude, 14);

        std::string body;
        body.reserve(64 + userId.size() + recordId.size());
        body += "userId=";       body += url_encode(userId);
        body += "&recordId=";    body += url_encode(recordId);
        body += "&posLongitude=";body += lon;
        body += "&posLatitude="; body += lat;
        return body;
    }
    std::wstring Headers() const {
        return MiniProgramHeadersForm();
    }
};

struct RandomLineReq {
    std::string userId;
    double posLongitude{};
    double posLatitude{};

    std::string BuildBody() const {
        std::string body;
        body.reserve(64 + userId.size());
        body += "userId="; body += url_encode(userId);
        body += "&posLongitude="; body += dtoa_fixed(posLongitude, 14);
        body += "&posLatitude=";  body += dtoa_fixed(posLatitude, 14);
        return body;
    }
    std::wstring Headers() const {
        return MiniProgramHeadersForm();
    }
};

struct RandomPointReq {
    std::string userId;
    std::string BuildBody() const {
        std::string body;
        body.reserve(32 + userId.size());
        body += "userId="; body += url_encode(userId);
        return body;
    }
    std::wstring Headers() const {
        return MiniProgramHeadersForm();
    }
};

struct EndRunReq {
    std::string userId;
    std::string runningRecordId;
    double mileage = 2.0;
    std::string speedAllocation = "10,00";
    double totalTime = 1.0;
    std::vector<Point> data;

    std::string BuildBody() const {
        json j;
        j["userId"] = userId;
        j["runningRecordId"] = runningRecordId;
        j["mileage"] = std::stod(dtoa_fixed(mileage, 2));
        j["speedAllocation"] = speedAllocation;
        j["totalTime"] = std::stod(dtoa_fixed(totalTime, 2));
        j["data"] = json::array();
        for (const auto& p : data) {
            json pt;
            pt["latitude"] = std::stod(dtoa_fixed(p.latitude, 6));
            pt["longitude"] = std::stod(dtoa_fixed(p.longitude, 6));
            j["data"].push_back(pt);
        }
        return j.dump();
    }

    std::wstring Headers() const {
        return MiniProgramHeadersJson();
    }
};


