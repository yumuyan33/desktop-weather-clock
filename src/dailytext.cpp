#include <Arduino.h>
#include "menu.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <string.h>
#include "WiFiUdp.h"
#include <WebServer.h>
// #include "..\lib\zlib\ArduinoZlib-main\src\ArduinoZlib.h"
#include <WiFiClientSecure.h>
#include "button.h"
#include "dispDirver.h"
#include "image.h"
#include "settings.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define BUTTON_NEXT 1
#define BUTTON_PREV 2

const String url_english = "http://apis.juhe.cn/fapigx/everyday/query";

static String content;
static String content_a;
static String content_b;
static String note;
static String note_a;
static String note_b;

String httpData;
int splitTextIntoLines(String text, int maxWidth, String lines[], int maxLines);
// 添加滚动控制变量（已移除滚动相关逻辑）
// static int scroll_x = 0;               // 滚动的X坐标位置
// static unsigned long scroll_timer = 0; // 滚动计时器
const int DISPLAY_WIDTH = 128; // OLED显示屏宽度
const int PADDING = 1;         // 文本之间的间距

int key_count();

void prase_english_data();

unsigned long time_now_1;

// 添加页面状态枚举
typedef enum
{
  PAGE_ENGLISH,
  PAGE_CHINESE
} DisplayPage;

static DisplayPage currentPage = PAGE_ENGLISH;
static bool needRefresh = true; // 用于标记是否需要刷新显示

void english_http_request(String url)
{
  // 检查是否配置了API密钥
  if (dailyKey.length() == 0)
  {
    Serial.println("[DailyText] API密钥未配置");
    return;
  }

  HTTPClient http;
  // 使用配置的密钥
  http.begin(url + "?key=" + dailyKey);

  int httpCode = http.GET();

  if (httpCode > 0)
  {
    if (httpCode == HTTP_CODE_OK)
    {
      Serial.printf("code=%d\n", httpCode);
      int len = -1;
      Serial.printf("size=%d\n", len);
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

      uint8_t buff[4096] = {0};
      httpData = http.getString();
      Serial.println("stream ok");

      prase_english_data();
    }
  }
  else
  {
    Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

static int i = 0;

void Show_dailytext(xpMenu Menu)
{
  // 初始化和数据获取
  if (i == 0)
  {
    OLED_ClearBuffer();

    // 检查API密钥是否配置
    if (dailyKey.length() == 0)
    {
      OLED_SetFont(u8g2_font_wqy12_t_gb2312a);
      OLED_DrawUTF8(20, 35, "请先配置API密钥");
      OLED_SendBuffer();
      delay(2000);
      return;
    }

    OLED_SetFont(u8g2_font_wqy12_t_gb2312a);
    OLED_DrawUTF8(20, 20, "每日英语打卡!");
    OLED_SendBuffer();
    delay(1000);
    english_http_request(url_english);
    i = 1;
  }

  // 使用菜单的按键扫描
  Menu_Direction buttonAction = Menu->dir;
  Serial.printf("Button Action from Menu->dir: %d\n", buttonAction);
  // 处理按键事件
  switch (buttonAction)
  {
  case MENU_DOWN:
    currentPage = (currentPage == PAGE_ENGLISH) ? PAGE_CHINESE : PAGE_ENGLISH;
    needRefresh = true;
    break;

  case MENU_ENTER:
    i = 0;
    currentPage = PAGE_ENGLISH;
    Change_MenuState(Menu, APP_QUIT);
    return;
    break;

  default:
    break;
  }

  // 更新显示
  if (needRefresh)
  {
    OLED_ClearBuffer();

    // 设置字体和字符宽度
    int charWidth;
    int lineHeight;
    if (currentPage == PAGE_ENGLISH)
    {
      OLED_SetFont(u8g2_font_6x13_me);
      charWidth = 6;   // 英文字符宽度
      lineHeight = 15; // 英文行高
    }
    else
    {
      OLED_SetFont(u8g2_font_wqy12_t_gb2312a);
      charWidth = 12;  // 中文字符宽度
      lineHeight = 14; // 中文行高
    }

    // 获取当前内容
    String currentText = (currentPage == PAGE_ENGLISH) ? content : note;

    // 计算每行可以容纳的字符数
    int maxWidth = DISPLAY_WIDTH - 8; // 左右各留4像素边距
    String lines[5];                  // 最多5行
    int numLines = splitTextIntoLines(currentText, maxWidth, lines, 5);

    // 分割并显示文本
    int yPosition = 12; // 起始Y坐标，留出顶部空间
    for (int line = 0; line < numLines; line++)
    {
      U8g2_text(4, yPosition + (line * lineHeight), lines[line].c_str());
    }

    // 显示页面指示器在左下角
    // OLED_SetFont(u8g2_font_6x10_tf);
    // const char *title = (currentPage == PAGE_ENGLISH) ? "English" : "中文翻译";
    // U8g2_text(2, VER_RES - 4, title);

    OLED_SendBuffer();
    needRefresh = false;
  }
}

/**
 * @brief 根据屏幕宽度和字符宽度分割文本行
 *
 * @param text 要显示的文本
 * @param maxWidth 最大宽度（像素）
 * @param lines 存储分割后的文本行
 * @param maxLines 最大行数
 * @return int 分割后的行数
 */
int splitTextIntoLines(String text, int maxWidth, String lines[], int maxLines)
{
  int numLines = 0;
  String currentLine = "";
  int currentWidth = 0;

  for (size_t i = 0; i < text.length(); i++)
  {
    char c = text.charAt(i);
    int charWidth;

    if ((uint8_t)c < 0x80)
    {
      // 英文字符
      charWidth = 6; // 英文字符宽度
    }
    else
    {
      // 中文字符
      charWidth = 12; // 中文字符宽度
      // 跳过UTF-8多字节字符的后续字节
      if (i + 2 < text.length())
      {
        i += 2;
      }
      else
      {
        break; // 避免越界
      }
    }

    // 检查是否需要换行
    if (currentWidth + charWidth > maxWidth)
    {
      if (currentLine.length() > 0)
      {
        lines[numLines++] = currentLine;
        if (numLines >= maxLines)
        {
          break;
        }
        currentLine = "";
        currentWidth = 0;
      }
    }

    // 添加字符到当前行
    if ((uint8_t)c < 0x80)
    {
      currentLine += c;
      currentWidth += charWidth;
    }
    else
    {
      // 添加完整的UTF-8字符
      currentLine += text.substring(i - 2, i + 1);
      currentWidth += charWidth;
    }
  }

  // 添加最后一行
  if (currentLine.length() > 0 && numLines < maxLines)
  {
    lines[numLines++] = currentLine;
  }

  return numLines;
}

void prase_english_data()
{
  DynamicJsonDocument txt(6144);
  DeserializationError error = deserializeJson(txt, httpData);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  content = txt["result"]["content"].as<String>();
  note = txt["result"]["note"].as<String>();
  txt.clear();

  // 重置相关状态
  needRefresh = true;
}
