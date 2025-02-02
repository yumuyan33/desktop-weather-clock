#include <Arduino.h>
#include <Wifi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <U8g2lib.h>
#include <ESPmDNS.h>
#include "menu.h"
#include "dispDirver.h"

WebServer server(80);
// String HTML_TITLE = "<!DOCTYPE html><html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\"><title>ESP8266网页配网</title>";
// String HTML_SCRIPT_ONE = "<script type=\"text/javascript\">function wifi(){var ssid = s.value;var password = p.value;var xmlhttp=new XMLHttpRequest();xmlhttp.open(\"GET\",\"/HandleWifi?ssid=\"+ssid+\"&password=\"+password,true);xmlhttp.send();xmlhttp.onload = function(e){alert(this.responseText);}}</script>";
// 更新HTML标题和样式部分

// String HTML_SCRIPT_TWO = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";
String HTML_HEAD_BODY_BEGIN = ""
                              "</head>"
                              "<body>"
                              "    <div class=\"container\">"
                              "        <p style=\"text-align: center; margin-bottom: 20px; color: #666;\">"
                              "            请输入wifi信息进行配网:"
                              "        </p>";
// String HTML_FORM_ONE = "<form>WiFi名称：<input id='s' name='s' type=\"text\" placeholder=\"请输入您WiFi的名称\"><br>WiFi密码：<input id='p' name='p' type=\"text\" placeholder=\"请输入您WiFi的密码\"><br><input type=\"button\" value=\"扫描\" onclick=\"window.location.href = '/HandleScanWifi'\"><input type=\"button\" value=\"连接\" onclick=\"wifi()\"></form>";
String HTML_BODY_HTML_END = "</body></html>";
/// @brief ///////////////////////////////
String HTML_FORM_ONE = ""
                       "    <div class=\"container\">" // 使用container类来居中对齐
                       "        <h2 style=\"text-align: center; color: #333; margin-bottom: 20px;\">ESP32 WiFi配置</h2>"
                       "        <div style=\"max-width: 300px; margin: 0 auto; padding: 20px; background: #fff; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1);\">"
                       "            <table style=\"width: 100%;\">"
                       "                <tr>"
                       "                    <td style=\"padding: 8px 0;\">WiFi名称：</td>"
                       "                    <td><input id=\"s\" type=\"text\" style=\"width: 100%; padding: 5px;\"></td>"
                       "                </tr>"
                       "                <tr>"
                       "                    <td style=\"padding: 8px 0;\">WiFi密码：</td>"
                       "                    <td><input id=\"p\" type=\"password\" style=\"width: 100%; padding: 5px;\"></td>"
                       "                </tr>"
                       "                <tr>"
                       "                    <td colspan=\"2\" style=\"text-align: center; padding-top: 15px;\">"
                       "                    <button onclick=\"window.location.href='/HandleScanWifi'\" style=\"margin: 0 5px;\">扫描</button>"
                       "                    <button onclick=\"wifi()\" style=\"margin: 0 5px;\">连接</button>"
                       "                    </td>"
                       "                </tr>"
                       "            </table>"
                       "        </div>"
                       "    </div>";

String HTML_TITLE = ""
                    "<!DOCTYPE html>"
                    "<html>"
                    "<head>"
                    "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
                    "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                    "    <meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">"
                    "    <title>ESP32配网页面</title>"
                    "    <style>"
                    "        body { "
                    "            font-family: Arial, sans-serif; "
                    "            margin: 0; "     // 移除body的margin
                    "            padding: 20px; " // 使用padding代替
                    "            background-color: #f0f0f0; "
                    "        }"
                    "        .container { "
                    "            max-width: 500px; "
                    "            margin: 0 auto; " // 这里的margin: 0 auto 使容器居中
                    "            padding: 20px; "
                    "            text-align: center; " // 添加文本居中
                    "        }"
                    "        h1, h2 { "
                    "            color: #333; "
                    "            text-align: center; "
                    "            margin-bottom: 20px; "
                    "        }"
                    "        input[type=\"text\"], input[type=\"password\"] { "
                    "            width: 100%; "
                    "            padding: 8px; "
                    "            margin: 5px 0; "
                    "            border: 1px solid #ddd; "
                    "            border-radius: 4px; "
                    "            box-sizing: border-box; "
                    "        }"
                    "        button { "
                    "            background-color: #4CAF50; "
                    "            color: white; "
                    "            padding: 10px 15px; "
                    "            border: none; "
                    "            border-radius: 4px; "
                    "            cursor: pointer; "
                    "            margin: 5px; "
                    "        }"
                    "        button:hover { "
                    "            background-color: #45a049; "
                    "        }"
                    "        table {"
                    "            margin: 0 auto; " // 表格居中
                    "            width: 100%; "
                    "        }"
                    "        td {"
                    "            padding: 8px 0; "
                    "        }"
                    "    </style>";

String HTML_SCRIPT_ONE = ""
                         "<script type=\"text/javascript\">"
                         "function wifi() {"
                         "var ssid = document.getElementById('s').value;"
                         "var password = document.getElementById('p').value;"
                         "var xmlhttp = new XMLHttpRequest();"
                         "xmlhttp.open(\"GET\", \"/HandleWifi?ssid=\"+ssid+\"&password=\"+password, true);"
                         "xmlhttp.send();"
                         "xmlhttp.onload = function(e) {"
                         "alert(this.responseText);"
                         "};"
                         "}"
                         "</script>";

String HTML_SCRIPT_TWO = ""
                         "<script>"
                         "function c(l) {"
                         "document.getElementById('s').value = l.innerText || l.textContent;"
                         "document.getElementById('p').focus();"
                         "}"
                         "</script>";

String HTML_FORM_TABLE_BEGIN = ""
                               "<table style=\"width: 100%; border-collapse: collapse; margin-top: 20px;\">"
                               "    <thead>"
                               "        <tr>"
                               "            <th style=\"padding: 10px; background-color: #4CAF50; color: white; text-align: center; width: 20%;\">序号</th>"
                               "            <th style=\"padding: 10px; background-color: #4CAF50; color: white; text-align: center; width: 50%;\">名称</th>"
                               "            <th style=\"padding: 10px; background-color: #4CAF50; color: white; text-align: center; width: 30%;\">强度</th>"
                               "        </tr>"
                               "    </thead>"
                               "    <tbody>";

String HTML_FORM_TABLE_END = ""
                             "    </tbody>"
                             "</table>";
String HTML_FORM_TABLE_CON = "";
String HTML_TABLE;

/*
首先，需要在法拉盛中存入wifi信息，在初始化中调用wifi信息连接，
如果连接失败，esp32会创建ap热点，用户连接热点后在web中上传当前
可用WiFi的名称和密码，esp32获取到后会再次进行连接wifi，如果依
然失败，会继续进入web配置wifi功能中
*/

bool autoConfig()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  Serial.print("AutoConfig Waiting......");
  int counter = 0;
  for (int i = 0; i < 20; i++)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("WiFi连接成功");
      Serial.print("IP地址: ");
      Serial.println(WiFi.localIP());

      // 显示连接成功和IP地址
      OLED_ClearBuffer();
      OLED_SetFont(u8g2_font_wqy12_t_gb2312);
      OLED_DrawUTF8(10, 20, "WiFi连接成功");

      String ip = WiFi.localIP().toString();
      char ipStr[32];
      snprintf(ipStr, sizeof(ipStr), "IP: %s", ip.c_str());
      OLED_DrawUTF8(10, 40, ipStr);

      OLED_SendBuffer();
      delay(2000); // 显示2秒

      return true;
    }
    else
    {
      delay(500);
      Serial.print(".");

      counter++;
    }
  }
  Serial.println("AutoConfig Faild!");
  return false;
}

void handleRoot()
{
  Serial.println("root page");
  String str = HTML_TITLE + HTML_SCRIPT_ONE + HTML_SCRIPT_TWO + HTML_HEAD_BODY_BEGIN + HTML_FORM_ONE + HTML_BODY_HTML_END;
  server.send(200, "text/html", str);
}

void HandleScanWifi()
{
  Serial.println("scan start");

  // String HTML_FORM_TABLE_BEGIN = "<table><head><tr><th>序号</th><th>名称</th><th>强度</th></tr></head><body>";

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
  {
    Serial.println("no networks found");
    HTML_TABLE = "NO WIFI !!!";
  }
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
      HTML_FORM_TABLE_CON = HTML_FORM_TABLE_CON + "<tr><td align=\"center\">" + String(i + 1) + "</td><td align=\"center\">" + "<a href='#p' onclick='c(this)'>" + WiFi.SSID(i) + "</a>" + "</td><td align=\"center\">" + WiFi.RSSI(i) + "</td></tr>";
    }

    HTML_TABLE = HTML_FORM_TABLE_BEGIN + HTML_FORM_TABLE_CON + HTML_FORM_TABLE_END;
  }
  Serial.println("");

  String scanstr = HTML_TITLE + HTML_SCRIPT_ONE + HTML_SCRIPT_TWO + HTML_HEAD_BODY_BEGIN + HTML_FORM_ONE + HTML_TABLE + HTML_BODY_HTML_END;

  server.send(200, "text/html", scanstr);
}

void HandleWifi()
{
  String wifis = server.arg("ssid");     // 从JavaScript发送的数据中找ssid的值
  String wifip = server.arg("password"); // 从JavaScript发送的数据中找password的值
  Serial.println("received:" + wifis);
  server.send(200, "text/html", "连接中..");
  WiFi.begin(wifis, wifip);
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void htmlConfig()
{
  WiFi.mode(WIFI_AP_STA); // 设置模式为AP+STA
  WiFi.softAP("wifi_esp32");

  IPAddress myIP = WiFi.softAPIP();

  if (MDNS.begin("clock"))
  {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/HandleWifi", HTTP_GET, HandleWifi);
  server.on("/HandleScanWifi", HandleScanWifi);
  server.onNotFound(handleNotFound); // 请求失败回调函数
  MDNS.addService("http", "tcp", 80);
  server.begin(); // 开启服务器
  Serial.println("HTTP server started");
  int counter = 0;

  while (1)
  {
    server.handleClient();
    // MDNS.update();
    MDNS.begin("clock");
    delay(500);

    OLED_ClearBuffer();
    OLED_SetFont(u8g2_font_wqy12_t_gb2312);
    Serial.println("正在web配网...\r\n");
    U8g2_text(0, 16, "打开手机或电脑");

    U8g2_text(0, 32, "连接wifi: wifi_esp32");

    U8g2_text(0, 48, "浏览器访问192.168.4.1");

    U8g2_text(0, 63, "输入wifi名称和密码连接");

    OLED_SendBuffer();
    counter++;
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("HtmlConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      Serial.println("HTML连接成功");
      break;
    }
  }
  server.close();
  WiFi.mode(WIFI_STA);
}