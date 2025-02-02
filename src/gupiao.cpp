#include <Arduino.h>
#include "menu.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <string.h>
#include "WiFiUdp.h"
#include <WebServer.h>
#include "..\lib\zlib\ArduinoZlib-main\src\ArduinoZlib.h"
#include <WiFiClientSecure.h>
#include "MenuConfig.h"
#include "button.h"
#include "dispDirver.h"
#include "image.h"
#include "gupiao.h"
#include "settings.h"

String key = "5416c2403ff4fc670b1db4a9ff9621da"; // myself
String SZIndexId = "sz399001";                   // 深证成指代码
String SHIndexId = "sh000001";                   // 上证指数代码
String baseURL = "https://web.juhe.cn/finance/stock/hs?key=" + key + "&type=";
String URLlink = "https://web.juhe.cn/finance/stock/hs?key=" + key + "&gid=" + stockId + "&type=";
String URLSZlink = "https://web.juhe.cn/finance/stock/sz?key=" + key + "&gid=" + stockId + "&type=0"; // 上证指数
String URLSHlink = "https://web.juhe.cn/finance/stock/sh?key=" + key + "&gid=" + stockId + "&type=1"; // 深证指数
// https://web.juhe.cn/finance/stock/hs?key=5416c2403ff4fc670b1db4a9ff9621da&gid=sh600519&type=
// https://web.juhe.cn/finance/stock/hs?key=5416c2403ff4fc670b1db4a9ff9621da&gid=sh601009&type=
//  存储股票数据的结构体
struct StockData
{
  // 个股数据
  String name;     // 股票名称
  String nowPri;   // 当前价格
  String increPer; // 涨跌百分比
  String increase; // 涨跌额

  // 指数数据（上证/深证）
  String szNowPri;   // 深证当前点数
  String szIncrePer; // 深证涨跌幅
  String shNowPri;   // 上证当前点数
  String shIncrePer; // 上证涨跌幅

  // 添加新字段
  String szTime;    // 深证时间
  String shTime;    // 上证时间
  String stockTime; // 个股时间

  // 添加更多价格信息
  String szOpenPri;    // 深证今开
  String szYesPri;     // 深证昨收
  String shOpenPri;    // 上证今开
  String shYesPri;     // 上证昨收
  String stockOpenPri; // 个股今开
  String stockYesPri;  // 个股昨收

  // 添加涨跌额字段
  String szIncrease; // 深证涨跌额
  String shIncrease; // 上证涨跌额
};

static StockData stockData;

// 当前显示的页面索引
static int currentPage = 0;
static const int TOTAL_PAGES = 3; // 总页数：上证指数、深证指数、个股信息
static unsigned long lastAutoSwitch = 0;
static const unsigned long AUTO_SWITCH_INTERVAL = 10000; // 10秒自动切换

// 添加时间记录变量
static unsigned long lastPageSwitch = 0;              // 上次页面切换时间
static unsigned long lastDataUpdate = 0;              // 上次数据更新时间
static const unsigned long PAGE_INTERVAL = 10000;     // 页面切换间隔（10秒）
static const unsigned long UPDATE_INTERVAL = 1800000; // 数据更新间隔（30分钟）

// 添加加载状态标志
static bool isLoading = true;

// 格式化时间显示
void formatTime(char *buffer)
{
  time_t now;
  time(&now);
  struct tm *timeinfo = localtime(&now);
  sprintf(buffer, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

// 获取个股信息
void getStockInfo()
{
  HTTPClient http;
  String url = baseURL + "&gid=" + stockId;

  Serial.println("[Stock] 获取个股数据...");
  Serial.println("[Stock] URL: " + url);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    String response = http.getString();
    // 打印原始JSON数据
    Serial.println("\n[Stock] 个股原始数据:");
    Serial.println(response);

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);

    if (!error && doc["resultcode"] == "200")
    {
      JsonObject data = doc["result"][0]["data"];
      stockData.name = data["name"].as<String>();
      stockData.nowPri = data["nowPri"].as<String>();
      stockData.increPer = data["increPer"].as<String>();
      stockData.increase = data["increase"].as<String>();
      stockData.stockTime = data["time"].as<String>();

      Serial.println("[Stock] 个股数据已更新");
    }
  }
  http.end();
}

// 获取深证指数
void getSZIndexInfo()
{
  HTTPClient http;
  String url = baseURL + "&gid=" + SZIndexId;

  Serial.println("[Stock] 获取深证指数...");
  Serial.println("[Stock] URL: " + url);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    String response = http.getString();
    // 打印原始JSON数据
    Serial.println("\n[Stock] 深证指数原始数据:");
    Serial.println(response);

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);

    if (!error)
    {
      if (doc["resultcode"] == "200")
      {
        JsonObject data = doc["result"][0]["data"];
        stockData.szNowPri = data["nowPri"].as<String>();
        stockData.szIncrePer = data["increPer"].as<String>();
        stockData.szTime = data["time"].as<String>();
        stockData.szIncrease = data["increase"].as<String>();

        Serial.println("\n[Stock] 深证指数更新:");
        Serial.printf("时间: %s\n", stockData.szTime.c_str());
        Serial.printf("当前点数: %s\n", stockData.szNowPri.c_str());
        Serial.printf("涨跌额: %s\n", stockData.szIncrease.c_str());
        Serial.printf("涨跌幅: %s%%\n", stockData.szIncrePer.c_str());
      }
    }
  }
  http.end();
}

// 获取上证指数
void getSHIndexInfo()
{
  HTTPClient http;
  String url = baseURL + "&gid=" + SHIndexId;

  Serial.println("[Stock] 获取上证指数...");
  Serial.println("[Stock] URL: " + url);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    String response = http.getString();
    // 打印原始JSON数据
    Serial.println("\n[Stock] 上证指数原始数据:");
    Serial.println(response);

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);

    if (!error)
    {
      if (doc["resultcode"] == "200")
      {
        JsonObject data = doc["result"][0]["data"];
        stockData.shNowPri = data["nowPri"].as<String>();
        stockData.shIncrePer = data["increPer"].as<String>();
        stockData.shTime = data["time"].as<String>();
        stockData.shIncrease = data["increase"].as<String>();

        Serial.println("\n[Stock] 上证指数更新:");
        Serial.printf("时间: %s\n", stockData.shTime.c_str());
        Serial.printf("当前点数: %s\n", stockData.shNowPri.c_str());
        Serial.printf("涨跌额: %s\n", stockData.shIncrease.c_str());
        Serial.printf("涨跌幅: %s%%\n", stockData.shIncrePer.c_str());
      }
    }
  }
  http.end();
}

// 更新所有股票数据
void updateStockData()
{
  getStockInfo();
  delay(1000); // 避免请求过快
  getSZIndexInfo();
  delay(1000);
  getSHIndexInfo();
}

// 显示指数页面（上证或深证）
void showIndexPage(xpMenu Menu, bool isShanghai)
{
  // 清屏并设置颜色
  OLED_ClearBuffer();
  OLED_SetDrawColor(&Menu->bgColor);
  OLED_DrawBox(0, 0, HOR_RES, VER_RES);
  uint8_t color = Menu->bgColor ^ 0x01;
  OLED_SetDrawColor(&color);

  // 设置字体
  OLED_SetFont(u8g2_font_wqy12_t_gb2312);

  // 显示标题
  OLED_DrawUTF8(2, 12, isShanghai ? "上证综指" : "深证成指");

  // 右上角显示时间
  String timeStr = isShanghai ? stockData.shTime : stockData.szTime;
  timeStr = timeStr.substring(11, 19); // 提取 HH:mm:ss
  int timeWidth = OLED_GetStrWidth(timeStr.c_str());
  // OLED_DrawStr(HOR_RES - timeWidth - 2, 12, timeStr.c_str()); // 右对齐，留2像素边距

  // 绘制分割线
  OLED_DrawHLine(0, 17, HOR_RES);

  // 获取数据
  String nowPri = isShanghai ? stockData.shNowPri : stockData.szNowPri;
  String increPer = isShanghai ? stockData.shIncrePer : stockData.szIncrePer;
  String increase = isShanghai ? stockData.shIncrease : stockData.szIncrease;

  // 显示"当前"文字
  OLED_SetFont(u8g2_font_wqy12_t_gb2312);
  OLED_DrawUTF8(2, 35, "当前:");

  // 显示当前点数（大字体，右移）
  OLED_SetFont(u8g2_font_7x14B_tr);
  OLED_DrawStr(35, 35, nowPri.c_str());

  // 显示涨跌信息
  char buffer[32];
  float change = increPer.toFloat();
  if (change > 0)
  {
    sprintf(buffer, "+%s [%s %%]", increase.c_str(), increPer.c_str());
  }
  else if (change < 0)
  {
    sprintf(buffer, "%s [%s %%]", increase.c_str(), increPer.c_str());
  }
  else
  {
    sprintf(buffer, "0.00 (0.00%%)");
  }
  OLED_DrawStr(2, 54, buffer);

  // OLED_SendBuffer();
}

// 显示个股页面
void showStockPage(xpMenu Menu)
{
  // 清屏并设置颜色
  OLED_ClearBuffer();
  OLED_SetDrawColor(&Menu->bgColor);
  OLED_DrawBox(0, 0, HOR_RES, VER_RES);
  uint8_t color = Menu->bgColor ^ 0x01;
  OLED_SetDrawColor(&color);

  // 设置字体
  OLED_SetFont(u8g2_font_wqy12_t_gb2312);

  // 显示标题
  OLED_DrawUTF8(2, 12, stockData.name.c_str());

  // 右上角显示时间
  String timeStr = stockData.stockTime;
  timeStr = timeStr.substring(11, 19); // 提取 HH:mm:ss
  int timeWidth = OLED_GetStrWidth(timeStr.c_str());
  OLED_DrawStr(HOR_RES - timeWidth - 2, 12, timeStr.c_str()); // 右对齐，留2像素边距

  // 绘制分割线
  OLED_DrawHLine(0, 17, HOR_RES);

  // 显示"当前"文字
  OLED_SetFont(u8g2_font_wqy12_t_gb2312);
  OLED_DrawUTF8(2, 35, "当前:");

  // 显示当前价格（大字体，右移）
  OLED_SetFont(u8g2_font_7x14B_tr);
  OLED_DrawStr(35, 35, stockData.nowPri.c_str());

  // 显示涨跌信息
  char buffer[32];
  float change = stockData.increPer.toFloat();
  if (change > 0)
  {
    sprintf(buffer, "+%s [%s %%]", stockData.increase.c_str(), stockData.increPer.c_str());
  }
  else
  {
    sprintf(buffer, "%s [%s %%]", stockData.increase.c_str(), stockData.increPer.c_str());
  }
  OLED_DrawStr(2, 54, buffer);
}

// 添加显示加载界面的函数
void showLoadingPage(xpMenu Menu)
{
  // 清屏并设置颜色
  OLED_ClearBuffer();
  OLED_SetDrawColor(&Menu->bgColor);
  OLED_DrawBox(0, 0, HOR_RES, VER_RES);
  uint8_t color = Menu->bgColor ^ 0x01;
  OLED_SetDrawColor(&color);

  // 设置字体
  OLED_SetFont(u8g2_font_wqy12_t_gb2312);

  // 显示加载文字
  OLED_DrawUTF8(20, 35, "正在加载数据...");

  OLED_SendBuffer();
}

// 修改主显示函数
void Show_gupiao(xpMenu Menu)
{
  // 检查是否配置了必要参数
  if (stockKey.length() == 0 || stockId.length() == 0)
  {
    OLED_ClearBuffer();
    OLED_SetFont(u8g2_font_wqy12_t_gb2312);
    OLED_DrawUTF8(20, 35, "请先配置股票参数");
    OLED_SendBuffer();
    return;
  }

  // 首次进入时，显示加载界面并开始获取数据
  if (isLoading)
  {
    showLoadingPage(Menu);
    updateStockData();
    isLoading = false;
    lastDataUpdate = millis();
    lastAutoSwitch = millis();
    return;
  }

  unsigned long currentTime = millis();

  // 处理按键切换
  if (Menu->dir == MENU_DOWN)
  {
    currentPage = (currentPage + 1) % TOTAL_PAGES;
    lastAutoSwitch = currentTime; // 重置自动切换计时器
  }
  else if (Menu->dir == MENU_UP)
  {
    currentPage = (currentPage + TOTAL_PAGES - 1) % TOTAL_PAGES;
    lastAutoSwitch = currentTime; // 重置自动切换计时器
  }

  // 处理自动切换
  if (currentTime - lastAutoSwitch >= AUTO_SWITCH_INTERVAL)
  {
    currentPage = (currentPage + 1) % TOTAL_PAGES;
    lastAutoSwitch = currentTime;
  }

  // 检查是否需要更新数据
  if ((currentTime - lastDataUpdate) >= UPDATE_INTERVAL)
  {
    Serial.println("\n[Stock] 开始更新数据...");
    updateStockData();
    lastDataUpdate = currentTime;
    Serial.println("[Stock] 数据更新完成");
  }

  // 根据当前页面显示相应内容
  switch (currentPage)
  {
  case 0:
    showIndexPage(Menu, true); // 显示上证指数
    break;
  case 1:
    showIndexPage(Menu, false); // 显示深证指数
    break;
  case 2:
    showStockPage(Menu); // 显示个股信息
    break;
  }

  // 统一在这里绘制页面指示点
  const int DOT_RADIUS = 2;
  const int DOT_SPACING = 8;
  const int DOT_Y = VER_RES - 4;
  const int FIRST_DOT_X = HOR_RES - (TOTAL_PAGES * DOT_SPACING + 4);

  for (int i = 0; i < TOTAL_PAGES; i++)
  {
    if (i == currentPage)
    {
      OLED_DrawDisc(FIRST_DOT_X + i * DOT_SPACING, DOT_Y, DOT_RADIUS);
    }
    else
    {
      OLED_DrawCircle(FIRST_DOT_X + i * DOT_SPACING, DOT_Y, DOT_RADIUS);
    }
  }

  // 统一在这里发送缓冲区
  OLED_SendBuffer();
}
