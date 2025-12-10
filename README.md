# 体锻打卡作弊器

一个基于底层C++ WinHTTP API实现的体锻打卡自动化解决方案，通过进程内存读取和HTTP请求模拟实现完整的打卡流程。

## 项目目的

练习c++网络编程以及逆向实践

## 项目概述

本项目采用纯C++实现 无第三方依赖 直接调用WindowsAPI完成网络通信和进程操作 通过逆向分析微信小程序获取API接口结构 实现自动化体锻打卡功能

## 技术架构

- winhttp库 实现api调用
- aobscan   查找内存获取用户唯一标识符以调用api接口

## 编译

### 系统要求
- Windows 10/11 (x64)
- Visual Studio 2019+ 或 MinGW-w64

### 编译配置
// 必需的链接库
#pragma comment(lib, "winhttp.lib")

// 编译器设置
C++11 标准或更高

### 编译命令
直接使用Visual Studio IDE编译

## 使用指北

1. 登录windows微信
2. 打开并登录**体锻打卡**微信小程序
3. 编译并运行本程序 程序将自动完成**1**次体锻打卡

## 免责声明

本项目仅供**学习和研究**使用，展示了：
- 底层C++网络编程技术
- Windows进程内存操作
- HTTP协议的底层实现

使用者需要：
- 遵守相关法律法规
- 尊重目标应用的服务条款
- 承担使用风险和责任

## 许可证

本项目采用 MIT 许可证，详见 LICENSE 文件。

## 致谢
[shnu-runing](https://github.com/xiaowanggua/shnurunningApp "shnu-runing") 作者xiaowanggua提供的灵感以及打卡点坐标数据  
[微信小程序逆向工具](https://github.com/strengthen/AppletReverseTool "微信小程序逆向工具") 使用该工具逆向体锻打卡小程序 了解打卡逻辑以及api接口url&数据结构  

## 存在问题

~~由于体锻打卡时段已结束,暂时无法测试程序功能() 有待后续测试~~

2025/10/09 更新
已测试并修复打卡功能, 可直接编译使用

## TBD

支持用户自定义打卡参数  
逆向userId生成算法  
etc.

---
