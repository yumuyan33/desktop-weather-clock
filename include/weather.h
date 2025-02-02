#ifndef __WEATHER_H__
#define __WEATHER_H__

#include <Arduino.h>
#include "menu.h"

#define WEATHER_KEY "your-api-key"
#define CITY_CODE "101220101"

void Show_weather(xpMenu Menu);
bool http_request(String url); // 更新函数声明

#endif
