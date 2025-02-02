#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>
#include "dispDirver.h"
#include "menu.h"

// 声明全局变量
extern int configValue;
extern String weatherKey;   // 和风天气API密钥
extern String cityLocation; // 城市编号
extern String dailyKey;     // 每日英语API密钥
extern String stockKey;     // 股票数据API密钥
extern String stockId;      // 股票代码
extern String biliUid;      // B站UID
extern String biliCookie;   // B站Cookie
extern WebServer webServer;

// 函数声明
void initSettings();
void handleWebServer();
void Show_settings(xpMenu Menu);
void saveSettings(); // 保存设置
void loadSettings(); // 加载设置

#endif
