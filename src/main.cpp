#include <Arduino.h>
#include <U8g2lib.h>
#include "menu.h"
#include "application.h"
#include "button.h"
#include "wifiConfig.h"
#include "Wire.h"
#include "dispDirver.h"
#include "image.h"
#include "settings.h"

#define pinSDA 19
#define pinSCL 21
#define DISPLAY_WIDTH 128 // OLED显示屏宽度
#define DISPLAY_HEIGHT 64 // OLED显示屏高度
xMenu menu;
int test;

void setup()
{
    Wire.begin(pinSDA, pinSCL, 200000);
    Serial.begin(115200, 134217756U, 3, 1);
    pinMode(5, INPUT);
    pinMode(22, INPUT);
    initButtons();
    Serial.println("test.....\r\n");
    Disp_Init();

    // 显示logo
    OLED_ClearBuffer();
    OLED_DrawXBMP(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, logo);
    OLED_SendBuffer();
    delay(2000);
    // 设置WiFi连接动画
    OLED_ClearBuffer();
    const int wifiStages = 4;

    // 设置中文字体和大小
    OLED_SetFont(u8g2_font_wqy12_t_gb2312a); // 使用12像素中文字体

    // 显示连接动画
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < wifiStages; j++)
        {
            OLED_ClearBuffer();
            U8g2_text(10, 16, "正在尝试连接网络....");
            OLED_DrawXBMP(48, 32, 32, 32, wifi_stages[j]);
            OLED_SendBuffer();
            delay(250);
        }
    }

    //  连接WiFi and 配网
    bool wifiConfig = autoConfig();
    if (wifiConfig == false)
    {
        htmlConfig();
    }
    Menu_Init(&menu);

    attachInterrupt(3, serialEventRun, 0);
    Serial.println("test.....end\r\n");

    // 初始化设置
    initSettings();
}

void loop()
{

    Menu_Loop(&menu);
}
