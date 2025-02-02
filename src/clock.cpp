#include <Arduino.h>
#include "menu.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "WiFiUdp.h"
#include "Time.h"
#include "TimeLib.h"
#include "clock.h"
#include "dispDirver.h"
#include "image.h"
#include "settings.h"

// 配置信息
bool weather_http_request(String url);
void parseJSON1(char *input, int inputLength);

// NTP服务器配置
IPAddress timeServer(120, 25, 115, 20); // 阿里云NTP服务器
const int timeZone = 8;                 // 北京时区
WiFiUDP Udp;

String url_now = "https://devapi.qweather.com/v7/weather/now";
// 外部变量声明
extern String weather_date_now;
extern String weather_temp_now;
extern int weather_icon_now;
extern unsigned int localPort;
uint8_t *outbuff;

// 静态变量
static String tim = "00:00";
static bool isFirstRun = true;
static unsigned long lastTimeUpdate = 0;
static unsigned long lastWeatherUpdate = 0;
static unsigned long dot_half_second = 0;
static const unsigned long HOUR_INTERVAL = 3600000; // 1小时的毫秒数
static const unsigned long HALF_SECOND = 500;       // 半秒的毫秒数
static bool dotBlink = true;                        // 时间分隔符闪烁状态

// NTP相关函数
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

// 添加新的状态变量
static bool isTimeInitialized = false;              // 时间是否已经初始化成功
static const unsigned long MINUTE_INTERVAL = 60000; // 1分钟的毫秒数

void sendNTPpacket(IPAddress &address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

time_t getNtpTime()
{
  const int MAX_RETRIES = 3; // 最大重试次数
  int retryCount = 0;

  while (retryCount < MAX_RETRIES)
  {
    while (Udp.parsePacket() > 0)
      ;
    Serial.printf("\n[Clock] NTP数据同步 >>>>>>>> 第%d次尝试\n", retryCount + 1);
    Serial.printf("[Clock] 正在连接NTP服务器: %s\n", timeServer.toString().c_str());
    sendNTPpacket(timeServer);

    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500)
    {
      int size = Udp.parsePacket();
      if (size >= NTP_PACKET_SIZE)
      {
        Serial.printf("[Clock] 收到NTP数据包，大小: %d bytes\n", size);
        Udp.read(packetBuffer, NTP_PACKET_SIZE);

        // 解析时间数据
        unsigned long secsSince1900;
        secsSince1900 = (unsigned long)packetBuffer[40] << 24;
        secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
        secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
        secsSince1900 |= (unsigned long)packetBuffer[43];

        time_t timeValue = secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;

        // 打印详细时间信息
        setTime(timeValue);
        Serial.printf("[Clock] 同步到的时间: %d-%02d-%02d %02d:%02d:%02d\n",
                      year(), month(), day(), hour(), minute(), second());
        Serial.println("[Clock] <<<<<<<< NTP同步完成\n");

        return timeValue;
      }
    }

    retryCount++;
    if (retryCount < MAX_RETRIES)
    {
      Serial.printf("[Clock] 第%d次同步失败，准备重试...\n", retryCount);
      delay(1000); // 等待1秒后重试
    }
  }

  Serial.println("[Clock] NTP服务器无响应");
  Serial.println("[Clock] <<<<<<<< 3次尝试后同步失败\n");
  return 0;
}

// 绘制天气图标
void drawWeatherIcon(int iconCode)
{
  const uint8_t *icon = nullptr;
  const char *weatherDesc = "未知天气";

  switch (iconCode)
  {
  case 100:
  case 150:
    icon = col_100;
    weatherDesc = "晴天";
    break;
  case 101:
  case 102:
  case 151:
    icon = col_102;
    weatherDesc = "多云";
    break;
  case 103:
  case 153:
    icon = col_103;
    break; // 晴间多云
  case 104:
  case 154:
    icon = col_104;
    break; // 阴
  case 300:
  case 301:
    icon = col_301;
    break; // 阵雨
  case 302:
  case 303:
    icon = col_302;
    break; // 雷阵雨
  case 304:
    icon = col_304;
    break; // 冰雹
  case 501:
  case 502:
    icon = col_501;
    break; // 雾天
  case 399:
  case 314:
  case 305:
  case 306:
  case 307:
  case 315:
  case 350:
  case 351:
    icon = col_307;
    break; // 雨
  default:
    icon = col_999;
    break; // 未知天气
  }

  if (icon)
  {
    OLED_DrawXBMP(0, 0, 40, 40, icon);
    //  Serial.printf("[Clock] 当前天气: %s (代码: %d)\n", weatherDesc, iconCode);
  }
}

// 更新时间显示
void updateTimeDisplay()
{
  String hourStr = (hour() < 10) ? "0" + String(hour()) : String(hour());
  String minStr = (minute() < 10) ? "0" + String(minute()) : String(minute());
  tim = hourStr + " " + minStr;
}

// 添加页面枚举
enum ClockPage
{
  PAGE_CLOCK = 0,  // 时钟主页面
  PAGE_SECOND = 1, // 第二页面
  PAGE_THIRD = 2   // 第三页面
};

// 添加静态变量跟踪当前页面
static ClockPage currentPage = PAGE_CLOCK;

// 添加动画控制变量
static unsigned long lastAnimUpdate = 0;
static const unsigned long ANIM_INTERVAL = 400; // 400ms per frame
static uint8_t currentFrame = 0;
static const uint8_t TOTAL_FRAMES = 3;    // 实际动画帧数为3
static bool isLeftFireworkPlaying = true; // 控制当前播放哪个烟花

// 添加猫咪动画控制变量
static unsigned long lastMaomiUpdate = 0;
static const unsigned long MAOMI_INTERVAL = 334; // 400ms per frame
static uint8_t maomiFrame = 0;
static const uint8_t MAOMI_TOTAL_FRAMES = 3;

// 封装烟花动画显示函数
void showFireworkAnim(int x, int y)
{
  // 每个帧的宽度是10，高度是10（根据数组大小计算）
  OLED_DrawXBMP(x, y, 10, 10, action_firework[currentFrame]);
}

// 封装时钟主页面的显示逻辑
void showMainClockPage(xpMenu Menu)
{
  OLED_ClearBuffer();

  // 显示天气图标和温度
  drawWeatherIcon(weather_icon_now);
  OLED_SetFont(u8g2_font_ncenB18_tf);
  int tempX = (weather_temp_now.length() == 1) ? 78 : 65;
  U8g2_text(tempX, 60, weather_temp_now.c_str());
  OLED_DrawXBMP(100, 36, 25, 25, col_ssd); // 温度单位

  // 显示日期
  OLED_SetFont(u8g2_font_6x12_mn);
  U8g2_text(1, 58, weather_date_now.substring(0, 10).c_str());

  // 显示时间
  OLED_SetFont(u8g2_font_fub25_tf);
  U8g2_text(40, 32, tim.c_str());
  U8g2_text(75, 32, dotBlink ? ":" : " ");

  OLED_SendBuffer();
}

// 辅助函数：根据数字获取对应的图片
const unsigned char *getNumberBitmap(int num)
{
  switch (num)
  {
  case 0:
    return num_zero;
  case 1:
    return num_one;
  case 2:
    return num_two;
  case 3:
    return num_three;
  case 4:
    return num_four;
  case 5:
    return num_five;
  case 6:
    return num_six;
  case 7:
    return num_seven;
  case 8:
    return num_eight;
  case 9:
    return num_nine;
  default:
    return num_zero;
  }
}

// 修改第二页面显示函数
void showSecondPage(xpMenu Menu)
{
  OLED_ClearBuffer();

  // 显示春联背景
  OLED_DrawXBMP(0, 0, 128, 64, img_chunlian);

  // 获取当前时间的小时和分钟
  int hours = hour();
  int minutes = minute();

  // 计算各个位的数字
  int hour_tens = hours / 10;
  int hour_ones = hours % 10;
  int min_tens = minutes / 10;
  int min_ones = minutes % 10;

  // 显示时间，每个数字占8x8像素，间隔2像素
  int x = 46; // 起始x坐标
  int y = 57; // y坐标

  // 显示小时
  OLED_DrawXBMP(x, y, 8, 8, getNumberBitmap(hour_tens));
  OLED_DrawXBMP(x + 7, y, 8, 8, getNumberBitmap(hour_ones));

  // 显示冒号（两个点）
  if (dotBlink)
  {
    OLED_DrawPixel(x + 15, y + 2); // 上点
    OLED_DrawPixel(x + 15, y + 4); // 下点
  }

  // 显示分钟
  OLED_DrawXBMP(x + 16, y, 8, 8, getNumberBitmap(min_tens));
  OLED_DrawXBMP(x + 24, y, 8, 8, getNumberBitmap(min_ones));

  // 显示烟花动画
  if (isLeftFireworkPlaying)
  {
    showFireworkAnim(6, 9); // 只显示左边的烟花
  }
  else
  {
    showFireworkAnim(101, 24); // 只显示右边的烟花
  }

  // 更新动画帧
  unsigned long currentTime = millis();
  if (currentTime - lastAnimUpdate >= ANIM_INTERVAL)
  {
    currentFrame = (currentFrame + 1) % TOTAL_FRAMES;

    // 如果左边的烟花播放完成，切换到右边的烟花
    if (currentFrame == 0 && isLeftFireworkPlaying)
    {
      isLeftFireworkPlaying = false;
    }
    // 如果右边的烟花也播放完成，重新开始循环
    else if (currentFrame == 0 && !isLeftFireworkPlaying)
    {
      isLeftFireworkPlaying = true;
    }

    lastAnimUpdate = currentTime;
  }

  OLED_SendBuffer();
}

// 修改第三页面显示函数
void showThirdPage(xpMenu Menu)
{
  OLED_ClearBuffer();

  // 根据当前帧显示对应的猫咪图片
  switch (maomiFrame)
  {
  case 0:
    OLED_DrawXBMP(0, 10, 128, 64, maomi_01);
    break;
  case 1:
    OLED_DrawXBMP(0, 10, 128, 64, maomi_02);
    break;
  case 2:
    OLED_DrawXBMP(0, 10, 128, 64, maomi_03);
    break;
  }

  // 显示时间
  OLED_SetFont(u8g2_font_courB14_tf); // 使用等宽字体

  // 格式化时间字符串
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", hour(), minute(), second());

  // 计算文本宽度以居中显示
  int strWidth = OLED_GetStrWidth(timeStr);
  int x = (128 - strWidth) / 2;

  // 在底部显示时间，白色背景黑色文字
  // OLED_SetDrawColor(1);                      // 白色
  // OLED_DrawBox(x - 2, 48, strWidth + 4, 16); // 绘制白色背景
  // OLED_SetDrawColor(0);                      // 黑色
  OLED_DrawStr(x + 20, 12, timeStr); // 显示时间文本

  // 更新动画帧
  unsigned long currentTime = millis();
  if (currentTime - lastMaomiUpdate >= MAOMI_INTERVAL)
  {
    maomiFrame = (maomiFrame + 1) % MAOMI_TOTAL_FRAMES;
    lastMaomiUpdate = currentTime;
  }

  OLED_SendBuffer();
}

// 添加WiFi连接状态检查
bool checkWiFiConnection()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[Clock] WiFi未连接，无法获取数据");
    return false;
  }
  return true;
}

// 显示加载提示
void showLoadingScreen(xpMenu Menu)
{
  OLED_ClearBuffer();
  OLED_SetDrawColor(&Menu->bgColor);
  OLED_DrawBox(0, 0, HOR_RES, VER_RES);
  uint8_t fgColor = Menu->bgColor ^ 0x01;
  OLED_SetDrawColor(&fgColor);

  OLED_SetFont(u8g2_font_wqy12_t_gb2312);
  OLED_DrawUTF8(20, 35, "正在加载数据...");

  OLED_SendBuffer();
}

// 添加内存监控函数实现
void printHeapInfo(const char *msg)
{
  Serial.printf("[Memory] %s - Free heap: %d, Largest block: %d\n",
                msg,
                ESP.getFreeHeap(),
                ESP.getMaxAllocHeap());
}

// 主显示函数
void Show_clock(xpMenu Menu)
{
  unsigned long currentTime = millis();

  // 首次运行初始化
  if (isFirstRun)
  {
    showLoadingScreen(Menu);

    // 检查WiFi连接
    if (!checkWiFiConnection())
    {
      return;
    }

    // 初始化UDP
    if (!Udp.begin(localPort))
    {
      return;
    }

    // 同步时间
    int retries = 3;
    while (retries-- > 0)
    {
      setSyncProvider(getNtpTime);
      if (timeStatus() == timeSet)
      {
        isTimeInitialized = true; // 标记时间已成功初始化
        break;
      }
      delay(1000);
    }

    // 获取天气
    weather_http_request(url_now);

    // 完成初始化
    updateTimeDisplay();
    lastTimeUpdate = lastWeatherUpdate = currentTime;
    isFirstRun = false;
  }

  // 定时更新（先检查WiFi连接）
  if (checkWiFiConnection())
  {
    // 天气更新逻辑
    if (currentTime - lastWeatherUpdate >= HOUR_INTERVAL)
    {
      showLoadingScreen(Menu);
      weather_http_request(url_now);
      lastWeatherUpdate = currentTime;
    }

    // 时间更新逻辑
    unsigned long timeUpdateInterval = isTimeInitialized ? HOUR_INTERVAL : MINUTE_INTERVAL;
    if (currentTime - lastTimeUpdate >= timeUpdateInterval)
    {
      setSyncProvider(getNtpTime);
      if (timeStatus() == timeSet)
      {
        isTimeInitialized = true; // 同步成功，转为1小时更新
        updateTimeDisplay();
      }
      // 如果同步失败且已初始化，继续使用当前时间
      // 如果未初始化，将在1分钟后重试
      lastTimeUpdate = currentTime;
    }
  }

  // 更新显示
  if (currentTime - dot_half_second >= HALF_SECOND)
  {
    dotBlink = !dotBlink;
    updateTimeDisplay(); // 每半秒更新一次时间显示
    dot_half_second = currentTime;
  }

  // 检查按钮状态并切换页面
  if (Menu->dir == MENU_DOWN)
  {
    currentPage = (ClockPage)((currentPage + 1) % 3);
  }
  else if (Menu->dir == MENU_UP)
  {
    currentPage = (ClockPage)((currentPage + 2) % 3); // +2 相当于 -1
  }

  // 根据当前页面显示对应内容
  switch (currentPage)
  {
  case PAGE_CLOCK:
    showMainClockPage(Menu);
    break;
  case PAGE_SECOND:
    showSecondPage(Menu);
    break;
  case PAGE_THIRD:
    showThirdPage(Menu);
    break;
  }
}

// 实现天气请求函数
bool weather_http_request(String url)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    return false;
  }

  HTTPClient http;
  http.begin(url + "?location=" + cityLocation + "&key=" + weatherKey);

  int httpCode = http.GET();

  if (httpCode > 0)
  {
    if (httpCode == HTTP_CODE_OK)
    {
      uint8_t buff[2048] = {0};
      WiFiClient *stream = http.getStreamPtr();

      if (http.connected())
      {
        size_t size = stream->available();
        if (size)
        {
          size_t readBytesSize = stream->readBytes(buff, size);
          outbuff = (uint8_t *)malloc(sizeof(uint8_t) * 15000);
          uint32_t outprintsize = 0;
          int result = ArduinoZlib::libmpq__decompress_zlib(buff, readBytesSize, outbuff, 15000, outprintsize);
          parseJSON1((char *)outbuff, outprintsize);
          free(outbuff);
        }
      }
    }
  }

  http.end();
  return true;
}

void parseJSON1(char *input, int inputLength)
{
  DynamicJsonDocument doc(3072);
  DeserializationError error = deserializeJson(doc, input, inputLength);

  if (error)
  {
    return;
  }

  JsonObject now = doc["now"];
  if (!now.isNull())
  {
    weather_date_now = now["obsTime"].as<String>();
    weather_temp_now = now["temp"].as<String>();
    weather_icon_now = now["icon"].as<int>();
  }
  Serial.printf("[Clock] 天气: %s, 温度: %s, 图标: %d\n", weather_date_now.c_str(), weather_temp_now.c_str(), weather_icon_now);
}