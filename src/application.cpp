#include "application.h"
#include "dispDirver.h"
#include "menu.h"
#include "MenuConfig.h"
#include "image.h"
#include "button.h"

// 首页
void Application::Draw_Home(xpMenu Menu)
{
    OLED_ClearBuffer();
    OLED_SetFont(MENU_FONT);
    OLED_DrawStr(0, Font_Hight * 2, "Welcome!");
    OLED_DrawStr(0, Font_Hight * 3, "Mini Desktop Display");
    OLED_DrawStr(0, Font_Hight * 4, "Press Any Key Start");
    OLED_DrawStr(50, Font_Hight * 5, "Version:2.1.9");
    OLED_SetFont(u8g2_font_wqy12_t_gb2312);
    U8g2_text(0, Font_Hight, "wifi连接成功 :-)");
    OLED_SendBuffer();
    // while (NONE == getButtonAction())
    // {
    //     handleButtons();
    // }
}

void Application::Show_Logo(xpMenu Menu)
{
    OLED_ClearBuffer();
    OLED_DrawXBMP(32, 0, 64, 64, logo);
    OLED_SendBuffer();
}

void Application::Show_weather(xpMenu Menu) // 每隔一小时刷新一次天气
{
}
