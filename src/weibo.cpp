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

#include "weibo.h"

// https://weibo.com/ajax/side/hotSearch

const String url_weibo = "https://weibo.com/ajax/side/hotSearch"; //
uint8_t *outdata;

String weibo_hotword[5];
String hotword_comb[5];
String payload;

byte data;

int x_position[5] = {0, 0, 0, 0, 0};
int label_x[5] = {0, 0, 0, 0, 0};

extern void Change_MenuState(xpMenu Menu, MenuState state);

void Show_weibo(xpMenu Menu);

void prase_weibo_data(xpMenu Menu);

static int num_key = 0;
int json_txt_num;
bool flag_get_resou = false;
unsigned long time_now;

void weibo_http_request(String url)
{
    HTTPClient http; // 创建对象
    http.begin(url); // 访问网站

    /*获取http响应码*/
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
            payload = http.getString();
            Serial.println("stream ok");
            flag_get_resou = true;
        }
        else
        {
            Serial.printf("httpCode is %d\r\n", httpCode);
        }
    }
    else
    {
        flag_get_resou = false;
        Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
}

static int i = 0;

void Show_weibo(xpMenu Menu)
{
    // 添加调试信息
    Serial.printf("Menu->dir = %d\n", Menu->dir);
    Serial.printf("Menu->menu_state = %d\n", Menu->menu_state);

    if (i == 0)
    {
        OLED_ClearBuffer();
        OLED_SetFont(u8g2_font_wqy12_t_gb2312a);
        U8g2_text(0, 12, "等待加载微博热搜....");
        OLED_SendBuffer();

        weibo_http_request(url_weibo);
        i = 1;
    }

    if (millis() - time_now >= 30 * 60 * 1000)
    {
        time_now = millis();
        weibo_http_request(url_weibo);
        for (int i = 0; i < 5; i++)
        {
            label_x[i] = 0;
        }
    }

    // 检测按键并更新num_key
    if (Menu->dir != MENU_NONE) // 只在有按键时处理
    {
        switch (Menu->dir)
        {
        case MENU_ENTER:
            i = 0;
            Change_MenuState(Menu, APP_QUIT);
            Menu->dir = MENU_NONE;  // 立即重置按键状态
            return;

        case MENU_DOWN:
            num_key += 5;
            if (num_key > 45)
            {
                num_key = 0;
            }
            // 重置所有文字位置
            for (int i = 0; i < 5; i++)
            {
                label_x[i] = 0;
            }
            Menu->dir = MENU_NONE;  // 重置按键状态
            break;

        case MENU_UP:
            num_key -= 5;
            if (num_key < 0)
            {
                num_key = 45;
            }
            // 重置所有文字位置
            for (int i = 0; i < 5; i++)
            {
                label_x[i] = 0;
            }
            Menu->dir = MENU_NONE;  // 重置按键状态
            break;
        }
    }

    prase_weibo_data(Menu);

    // 处理文字滚动
    for (int j = 0; j < 5; j++)
    {
        int textWidth = weibo_hotword[j].length();
        if (textWidth > 33)
        {
            weibo_hotword[j] = weibo_hotword[j] + ">" + weibo_hotword[j];
            if (label_x[j] / 4 + 3 > -textWidth)
            {
                label_x[j] = label_x[j] - 2;
            }
            else
            {
                label_x[j] = 0;
            }
        }
        delay(2);
    }

    // 显示内容
    if (flag_get_resou == true)
    {
        OLED_ClearBuffer();
        OLED_SetFont(u8g2_font_wqy12_t_gb2312a);
        U8g2_text(label_x[0], 12 * 1, weibo_hotword[0].c_str());
        U8g2_text(label_x[1], 12 * 2, weibo_hotword[1].c_str());
        U8g2_text(label_x[2], 12 * 3, weibo_hotword[2].c_str());
        U8g2_text(label_x[3], 12 * 4, weibo_hotword[3].c_str());
        U8g2_text(label_x[4], 12 * 5, weibo_hotword[4].c_str());
        OLED_SendBuffer();
    }
    else
    {
        OLED_ClearBuffer();
        OLED_SetFont(u8g2_font_wqy12_t_gb2312a);
        U8g2_text(0, 0, "加载失败....");
        OLED_SendBuffer();
    }
}

void prase_weibo_data(xpMenu Menu)
{
    json_txt_num = num_key;
    DynamicJsonDocument txt(6144);

    DeserializationError error = deserializeJson(txt, payload);
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }
    /*从解析的数据中获取相关信息*/
    weibo_hotword[0] = txt["data"]["realtime"][json_txt_num]["word"].as<String>();
    weibo_hotword[1] = txt["data"]["realtime"][json_txt_num + 1]["word"].as<String>();
    weibo_hotword[2] = txt["data"]["realtime"][json_txt_num + 2]["word"].as<String>();
    weibo_hotword[3] = txt["data"]["realtime"][json_txt_num + 3]["word"].as<String>();
    weibo_hotword[4] = txt["data"]["realtime"][json_txt_num + 4]["word"].as<String>();

    txt.clear();

    // Serial.printf("%s\r\n",weibo_hotword[0]);
    // Serial.printf("%s\r\n",weibo_hotword[1]);
    // Serial.printf("%s\r\n",weibo_hotword[2]);
    // Serial.printf("%s\r\n",weibo_hotword[3]);
    // Serial.printf("%s\r\n",weibo_hotword[4]);
}
