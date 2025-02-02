#include "settings.h"

// 定义全局变量
int configValue = 0;
String weatherKey = "";   // 默认为空
String cityLocation = ""; // 默认为空
String dailyKey = "";     // 每日英语API密钥
String stockKey = "";     // 股票数据API密钥
String stockId = "";      // 股票代码
String biliUid = "";      // B站UID
String biliCookie = "";   // B站Cookie
WebServer webServer(80);

// 创建 Preferences 对象
Preferences preferences;

// 保存设置到 Flash
void saveSettings()
{
  preferences.begin("weather", false); // 打开命名空间 "weather"

  preferences.putInt("configValue", configValue);
  preferences.putString("weatherKey", weatherKey);
  preferences.putString("cityLocation", cityLocation);
  preferences.putString("dailyKey", dailyKey);
  preferences.putString("stockKey", stockKey);
  preferences.putString("stockId", stockId);
  preferences.putString("biliUid", biliUid);
  preferences.putString("biliCookie", biliCookie);

  preferences.end();
  Serial.println("[Settings] 配置已保存到Flash");
}

// 从 Flash 加载设置
void loadSettings()
{
  preferences.begin("weather", true); // 只读模式打开

  configValue = preferences.getInt("configValue", 0);
  weatherKey = preferences.getString("weatherKey", "");
  cityLocation = preferences.getString("cityLocation", "");
  dailyKey = preferences.getString("dailyKey", "");
  stockKey = preferences.getString("stockKey", "");
  stockId = preferences.getString("stockId", "sh600519"); // 默认值
  biliUid = preferences.getString("biliUid", "");
  biliCookie = preferences.getString("biliCookie", "");

  preferences.end();
  Serial.println("[Settings] 从Flash加载配置完成");
  Serial.printf("[Settings] configValue: %d\n", configValue);
  Serial.printf("[Settings] weatherKey: %s\n", weatherKey.c_str());
  Serial.printf("[Settings] cityLocation: %s\n", cityLocation.c_str());
  Serial.printf("[Settings] dailyKey: %s\n", dailyKey.c_str());
  Serial.printf("[Settings] stockKey: %s\n", stockKey.c_str());
  Serial.printf("[Settings] stockId: %s\n", stockId.c_str());
  Serial.printf("[Settings] biliUid: %s\n", biliUid.c_str());
}

// 更新HTML页面模板，添加新的配置项
const char *html_page = R"(
<!DOCTYPE html>
<html>
<head>
    <title>设备配置</title>
    <meta charset='UTF-8'>
    <style>
        body { font-family: Arial; margin: 20px; }
        .container { max-width: 400px; margin: 0 auto; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; }
        input { width: 100%%; padding: 8px; margin: 5px 0; box-sizing: border-box; }
        button { background: #4CAF50; color: white; padding: 10px; border: none; width: 100%%; }
        .current { color: #666; margin-top: 5px; }
        .section { border-bottom: 1px solid #eee; padding-bottom: 15px; margin-bottom: 15px; }
    </style>
</head>
<body>
    <div class='container'>
        <h2>设备配置</h2>
        <form action='/update' method='post'>
            <div class="section">
                <h3>天气设置</h3>
                <div class="form-group">
                    <label>和风天气API密钥:</label>
                    <input type='text' name='key' value='%s' placeholder="请输入API密钥">
                    <div class="current">当前密钥: %s</div>
                </div>
                <div class="form-group">
                    <label>城市编号:</label>
                    <input type='text' name='location' value='%s' placeholder="请输入城市编号">
                    <div class="current">当前城市编号: %s</div>
                </div>
            </div>

            <div class="section">
                <h3>每日英语设置</h3>
                <div class="form-group">
                    <label>每日英语API密钥:</label>
                    <input type='text' name='dailyKey' value='%s' placeholder="请输入每日英语API密钥">
                    <div class="current">当前密钥: %s</div>
                </div>
            </div>

            <div class="section">
                <h3>股票数据设置</h3>
                <div class="form-group">
                    <label>股票数据API密钥:</label>
                    <input type='text' name='stockKey' value='%s' placeholder="请输入股票数据API密钥">
                    <div class="current">当前密钥: %s</div>
                </div>
                <div class="form-group">
                    <label>股票代码:</label>
                    <input type='text' name='stockId' value='%s' placeholder="例如: sh600519">
                    <div class="current">当前股票代码: %s</div>
                    <div class="hint">格式: sh/sz + 股票代码，如 sh600519</div>
                </div>
            </div>

            <div class="section">
                <h3>哔哩哔哩设置</h3>
                <div class="form-group">
                    <label>B站UID:</label>
                    <input type='text' name='biliUid' value='%s' placeholder="请输入B站UID">
                    <div class="current">当前UID: %s</div>
                </div>
                <div class="form-group">
                    <label>B站Cookie:</label>
                    <input type='text' name='biliCookie' value='%s' placeholder="请输入B站Cookie">
                    <div class="current">当前Cookie: %s</div>
                    <div class="hint">请从浏览器复制完整的Cookie</div>
                </div>
            </div>
            
            <button type='submit'>更新配置</button>
        </form>
    </div>
</body>
</html>
)";

// 修改处理函数
void handleSettingsRoot()
{
  char temp[4096];
  snprintf(temp, sizeof(temp), html_page,
           weatherKey.c_str(), weatherKey.c_str(),
           cityLocation.c_str(), cityLocation.c_str(),
           dailyKey.c_str(), dailyKey.c_str(),
           stockKey.c_str(), stockKey.c_str(),
           stockId.c_str(), stockId.c_str(),
           biliUid.c_str(), biliUid.c_str(),
           biliCookie.c_str(), biliCookie.c_str());
  webServer.send(200, "text/html", temp);
}

// 更新处理更新请求的函数
void handleUpdate()
{
  if (webServer.hasArg("key"))
  {
    weatherKey = webServer.arg("key");
  }
  if (webServer.hasArg("location"))
  {
    cityLocation = webServer.arg("location");
  }
  if (webServer.hasArg("dailyKey"))
  {
    dailyKey = webServer.arg("dailyKey");
  }
  if (webServer.hasArg("stockKey"))
  {
    stockKey = webServer.arg("stockKey");
  }
  if (webServer.hasArg("stockId"))
  {
    String newStockId = webServer.arg("stockId");
    // 简单的格式验证
    if (newStockId.startsWith("sh") || newStockId.startsWith("sz"))
    {
      stockId = newStockId;
      Serial.printf("[Settings] 更新股票代码: %s\n", stockId.c_str());
    }
  }
  if (webServer.hasArg("biliUid"))
  {
    biliUid = webServer.arg("biliUid");
    Serial.printf("[Settings] 更新B站UID: %s\n", biliUid.c_str());
  }
  if (webServer.hasArg("biliCookie"))
  {
    biliCookie = webServer.arg("biliCookie");
    Serial.println("[Settings] 更新B站Cookie");
  }

  // 保存更新后的设置
  saveSettings();

  webServer.sendHeader("Location", "/settings");
  webServer.send(302, "text/plain", "Updated");
}

// 初始化设置
void initSettings()
{
  // 加载保存的设置
  loadSettings();

  // 设置路由
  webServer.on("/settings", HTTP_GET, handleSettingsRoot);
  webServer.on("/update", HTTP_POST, handleUpdate);

  // 启动服务器
  webServer.begin();
  Serial.println("[Settings] Web服务器已启动");
}

// 处理Web服务器请求
void handleWebServer()
{
  webServer.handleClient();
}

// 添加滚动相关变量
static int scroll_x = 0;
static unsigned long scroll_timer = 0;
const int SCROLL_INTERVAL = 40;      // 滚动速度
const int SCROLL_RESET_DELAY = 1000; // 重置延迟

// 修改显示函数
void Show_settings(xpMenu Menu)
{
  handleWebServer();

  OLED_ClearBuffer();
  OLED_SetFont(u8g2_font_wqy12_t_gb2312);

  // 准备滚动文本
  String ip = WiFi.localIP().toString();
  char ipStr[64];
  snprintf(ipStr, sizeof(ipStr), "打开浏览器访问: %s/settings", ip.c_str());

  // 计算文本宽度
  int textWidth = OLED_GetStrWidth(ipStr);
  int screenWidth = 128; // OLED屏幕宽度

  // 处理滚动
  unsigned long currentTime = millis();
  if (currentTime - scroll_timer >= SCROLL_INTERVAL)
  {
    scroll_timer = currentTime;
    scroll_x--;

    // 当文本完全滚出屏幕后，等待一段时间再重置
    if (-scroll_x >= textWidth + screenWidth / 4)
    {
      delay(SCROLL_RESET_DELAY);
      scroll_x = screenWidth;
    }
  }

  // 显示滚动文本
  OLED_DrawUTF8(scroll_x, 12, ipStr);

  // 显示配置状态（整齐排列）
  const int LEFT_COLUMN_X = 5;   // 左列起始位置
  const int RIGHT_COLUMN_X = 70; // 右列起始位置
  const int ROW1_Y = 30;         // 第一行Y坐标
  const int ROW2_Y = 45;         // 第二行Y坐标
  const int ROW3_Y = 60;         // 第三行Y坐标

  // 左列显示
  if (weatherKey.length() > 0)
  {
    OLED_DrawUTF8(LEFT_COLUMN_X, ROW1_Y, "天气已配置");
  }
  else
  {
    OLED_DrawUTF8(LEFT_COLUMN_X, ROW1_Y, "天气未配置");
  }

  if (dailyKey.length() > 0)
  {
    OLED_DrawUTF8(LEFT_COLUMN_X, ROW2_Y, "英语已配置");
  }
  else
  {
    OLED_DrawUTF8(LEFT_COLUMN_X, ROW2_Y, "英语未配置");
  }

  if (stockKey.length() > 0)
  {
    OLED_DrawUTF8(LEFT_COLUMN_X, ROW3_Y, "股票已配置");
  }
  else
  {
    OLED_DrawUTF8(LEFT_COLUMN_X, ROW3_Y, "股票未配置");
  }

  // 右列显示
  if (biliUid.length() > 0)
  {
    OLED_DrawUTF8(RIGHT_COLUMN_X, ROW1_Y, "B站已配置");
  }
  else
  {
    OLED_DrawUTF8(RIGHT_COLUMN_X, ROW1_Y, "B站未配置");
  }

  OLED_SetFont(u8g2_font_streamline_all_t);
  OLED_DrawGlyph(78, VER_RES - 4, 0x0081);

  OLED_SendBuffer();
}
