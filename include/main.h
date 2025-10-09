// main.h
#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <charconv>

#include "aobscan.h"
#include "http.h"


// 使用https://github.com/strengthen/AppletReverseTool逆向体锻打卡小程序 获取api接口以及相应请求数据结构

static inline std::string dtoa_fixed(double v, int precision = 14) {
    char buf[64];
    auto res = std::to_chars(std::begin(buf), std::end(buf),
        v, std::chars_format::fixed, precision);
    return std::string(buf, res.ptr);
}

inline constexpr double Coords[4][2] = {
    {121.51818147705078, 30.837721567871094},
    {121.52092847705076, 30.834883567871294},
    {121.51926147705322, 30.835872567871354},
    {121.51749847705033, 30.835306567871091},
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
        return L"Content-Type: application/x-www-form-urlencoded\r\n";
    }
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
        return L"Content-Type: application/x-www-form-urlencoded\r\n";
    }
};

struct EndRunReq {
    std::string userId;
    std::string runningRecordId;
    unsigned int mileage = 2;
    unsigned int speedAllocation = 0;
    unsigned int totalTime = 1;

    std::string BuildBody() const {
        std::string body;
		body.reserve(256 + userId.size() + runningRecordId.size());
		body += "{\"userId\":\""; body += json_escape(userId); body += "\"";
		body += ",\"runningRecordId\":\""; body += json_escape(runningRecordId); body += "\"";
		body += ",\"mileage\":"; body += std::to_string(mileage);
		body += ",\"speedAllocation\":"; body += std::to_string(speedAllocation);
		body += ",\"totalTime\":"; body += std::to_string(totalTime);
		body += ",\"data\":[]}";
        return body;
    }

    std::wstring Headers() const {
        return L"Content-Type: application/json; charset=UTF-8\r\n";
    }
};


