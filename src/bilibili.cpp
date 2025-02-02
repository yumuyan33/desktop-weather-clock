#include <Arduino.h>
#include "menu.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "dispDirver.h"
#include "image.h"
#include "bilibili.h"
#include "settings.h"

// 定义B站数据结构
struct BilibiliStats
{
  int follower; // 粉丝数
  int view;     // 播放数
  int likes;    // 点赞数
};

// 配置信息
const char *FOLLOWER_URL = "https://api.bilibili.com/x/relation/stat"; // 获取粉丝数
const char *VIEW_URL = "https://api.bilibili.com/x/space/upstat";      // 获取播放量和点赞
const char *USER_AGENT = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36";
const char *REFERER = "https://space.bilibili.com/260707594";

// 必要的Cookie字段
const char *COOKIE = "SESSDATA=dc1e3d38%2C1752322731%2C0bf8d%2A12CjAN4O7mMGJWu5A_01SHPpjTB5iunOyJSxq1jrOOIUm5XmcT1yCK5mRSADR1_Sw5-NwSVmRLUUZ2Y1F6RGlnWGlyUHAzMnJRLXFFMURsLU45QTVMVEpRYzB1d21CclBBNlBMLTlNY1VjajVHNVpEY3Njdl9RWWFGRlJjUXdkd0RFVjlhdFhkYlBRIIEC";

// 静态变量
static BilibiliStats stats = {0, 0, 0};
static bool isLoading = true;
static unsigned long lastUpdate = 0;
static const unsigned long UPDATE_INTERVAL = 3600000; // 1小时更新一次

// 显示加载页面
void showBiliLoadingPage(xpMenu Menu)
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

// HTTP请求辅助函数
bool makeHttpRequest(const char *baseUrl, String params, JsonDocument &doc)
{
  String url = String(baseUrl) + "?" + params;

  HTTPClient http;
  http.begin(url);

  // 添加必要的请求头
  http.addHeader("User-Agent", USER_AGENT);
  http.addHeader("Cookie", COOKIE);
  http.addHeader("Referer", REFERER);

  Serial.printf("[Bilibili] 请求URL: %s\n", url.c_str());
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    String response = http.getString();
    Serial.println("[Bilibili] 响应数据:");
    Serial.println(response);

    DeserializationError error = deserializeJson(doc, response);
    if (!error)
    {
      // 检查API返回码
      int code = doc["code"].as<int>();
      if (code == 0)
      {
        http.end();
        return true;
      }
      else
      {
        Serial.printf("[Bilibili] API错误: %d - %s\n",
                      code, doc["message"].as<const char *>());
      }
    }
    else
    {
      Serial.printf("[Bilibili] JSON解析失败: %s\n", error.c_str());
    }
  }
  else
  {
    Serial.printf("[Bilibili] HTTP请求失败: %d\n", httpCode);
  }

  http.end();
  return false;
}

// 获取B站数据
void updateBilibiliStats()
{
  Serial.println("[Bilibili] 开始更新数据...");

  // 1. 获取粉丝数
  DynamicJsonDocument doc1(1024);
  if (makeHttpRequest(FOLLOWER_URL, "vmid=" + String(biliUid), doc1))
  {
    stats.follower = doc1["data"]["follower"].as<int>();
    Serial.printf("[Bilibili] 粉丝数: %d\n", stats.follower);
  }

  delay(500); // 请求间隔

  // 2. 获取播放量和点赞数
  DynamicJsonDocument doc2(2048);
  if (makeHttpRequest(VIEW_URL, "mid=" + String(biliUid), doc2))
  {
    JsonObject data = doc2["data"];
    stats.view = data["archive"]["view"].as<int>();
    stats.likes = data["likes"].as<int>();
    Serial.printf("[Bilibili] 播放量: %d, 点赞: %d\n",
                  stats.view, stats.likes);
  }

  Serial.printf("[Bilibili] 数据更新完成\n");
}

// 格式化数字显示（添加万单位）
void formatNumber(int number, char *buffer)
{
  if (number >= 10000)
  {
    float num = number / 10000.0f;
    sprintf(buffer, "%.1fw", num);
  }
  else
  {
    sprintf(buffer, "%d", number);
  }
}

// 计算文本居中位置
int calculateCenterX(const char *str, int baseX, int iconWidth = 30)
{
  int strWidth = strlen(str) * 7; // 假设字体宽度为7像素
  return baseX + (iconWidth - strWidth) / 2;
}

// 显示主界面
void showMainPage(xpMenu Menu)
{
  OLED_ClearBuffer();
  OLED_SetDrawColor(&Menu->bgColor);
  OLED_DrawBox(0, 0, HOR_RES, VER_RES);
  uint8_t textColor = Menu->bgColor ^ 0x01;
  OLED_SetDrawColor(&textColor);

  // 布局常量
  const int ICON_SIZE = 30;
  const int ICON_Y = 5;
  const int TEXT_Y = ICON_Y + ICON_SIZE + 15;
  const int ICON_SPACING = 39;
  const int FIRST_ICON_X = 10;

  // 绘制三个图标
  const uint8_t *icons[] = {img_foller, img_view, img_likes};
  int values[] = {stats.follower, stats.view, stats.likes};

  for (int i = 0; i < 3; i++)
  {
    int iconX = FIRST_ICON_X + i * ICON_SPACING;
    OLED_DrawXBMP(iconX, ICON_Y, ICON_SIZE, ICON_SIZE, icons[i]);

    char buffer[10];
    formatNumber(values[i], buffer);
    OLED_SetFont(u8g2_font_7x14B_tr);
    OLED_DrawStr(calculateCenterX(buffer, iconX), TEXT_Y, buffer);
  }

  OLED_SendBuffer();
}

// 主显示函数
void Show_bilibili(xpMenu Menu)
{
  unsigned long currentTime = millis();

  // 检查是否配置了必要参数
  if (biliUid.length() == 0 || biliCookie.length() == 0)
  {
    OLED_ClearBuffer();
    OLED_SetFont(u8g2_font_wqy12_t_gb2312);
    OLED_DrawUTF8(20, 35, "请先配置B站参数");
    OLED_SendBuffer();
    return;
  }

  // 首次加载或需要更新数据
  if (isLoading || (currentTime - lastUpdate >= UPDATE_INTERVAL))
  {
    showBiliLoadingPage(Menu);
    updateBilibiliStats();
    lastUpdate = currentTime;
    isLoading = false;
    return;
  }

  showMainPage(Menu);
}