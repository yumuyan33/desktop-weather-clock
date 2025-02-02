#include "menuConfig.h"
#include "menu.h"
#include "dispDirver.h"
#include "image.h"
#include "application.h"

/* Page*/
xPage Home_Page, System_Page;
/* item */
xItem HomeHead_Item, SystemHead_Item, System_Item, Image_Item, Weather_Item, Clock_Item, DailyText_Item,
    Weibo_Item, Bilibili_Item, Game_Item, Heart_Item, Github_Item, PCSpeed_Item, Note_Item, Gupiao_Item, Settings_Item;
xItem Contrast_Item, Power_Item;
xItem Wave_Item;

extern int test;

/**
 * 在此建立所需显示或更改的数据
 * 无参数
 * 无返回值
 */
void Create_Parameter(void)
{
    static int Contrast = 255;
    static data_t Contrast_data;
    Contrast_data.name = "Contrast";
    Contrast_data.ptr = &Contrast;
    Contrast_data.function = OLED_SetContrast;
    Contrast_data.Function_Type = STEP_EXECUTE;
    Contrast_data.Data_Type = DATA_INT;
    Contrast_data.Operate_Type = READ_WRITE;
    Contrast_data.max = 255;
    Contrast_data.min = 0;
    Contrast_data.step = 2;
    static element_t Contrast_element;
    Contrast_element.data = &Contrast_data;
    Create_element(&Contrast_Item, &Contrast_element);

    static uint8_t power = true;
    static data_t Power_switch_data;
    Power_switch_data.ptr = &power;
    Power_switch_data.function = OLED_SetPowerSave;
    Power_switch_data.Data_Type = DATA_SWITCH;
    Power_switch_data.Operate_Type = READ_WRITE;
    static element_t Power_element;
    Power_element.data = &Power_switch_data;
    Create_element(&Power_Item, &Power_element);

    static data_t Wave_data;
    Wave_data.name = "Wave";
    Wave_data.ptr = &test;
    Wave_data.Data_Type = DATA_INT;
    Wave_data.max = 360;
    Wave_data.min = 0;
    static element_t Wave_element;
    Wave_element.data = &Wave_data;
    Create_element(&Wave_Item, &Wave_element);

    static data_t Heart_data;
    Heart_data.name = "--Heart";
    Heart_data.ptr = &test;
    Heart_data.Data_Type = DATA_INT;
    Heart_data.max = 999;
    Heart_data.min = 0;
    static element_t Heart_element;
    Heart_element.data = &Heart_data;
    Create_element(&Heart_Item, &Heart_element);
}

/**
 * 在此建立所需显示或更改的文本
 * 无参数
 * 无返回值
 */
void Create_Text(void)
{
    static text_t github_text;
    github_text.font = MENU_FONT;
    github_text.font_hight = Font_Hight;
    github_text.font_width = Font_Width;
    github_text.ptr = "aaa";
    static element_t github_element;
    github_element.text = &github_text;
    Create_element(&Github_Item, &github_element);
}

/*
 * 菜单构建函数
 * 该函数不接受参数，也不返回任何值。
 * 功能：静态地构建一个菜单系统。
 */
void Create_MenuTree(xpMenu Menu)
{
    AddPage("[HomePage]", &Home_Page, IMAGE);
    AddItem("[HomePage]", ONCE_FUNCTION, NULL, &HomeHead_Item, &Home_Page, NULL, Application::Draw_Home);
    AddItem(" -Clock", CLOCK, logo_allArray[0], &Clock_Item, &Home_Page, NULL, NULL);
    AddItem(" -Weather", WEATHER, logo_allArray[1], &Weather_Item, &Home_Page, NULL, NULL);
    AddItem(" -Weibo", WEIBO, logo_allArray[2], &Weibo_Item, &Home_Page, NULL, NULL);
    AddItem(" -English text", DAILYTEXT, logo_allArray[3], &DailyText_Item, &Home_Page, NULL, NULL);
    AddItem(" -Game", GAME, logo_allArray[4], &Game_Item, &Home_Page, NULL, NULL);
    AddItem(" -Heartbeat", HEART, logo_allArray[5], &Heart_Item, &Home_Page, NULL, NULL);
    AddItem(" -Bilibili", BILIBILI, logo_allArray[6], &Bilibili_Item, &Home_Page, NULL, NULL);
    AddItem(" -PC Speed", PCSPEED, logo_allArray[7], &PCSpeed_Item, &Home_Page, NULL, NULL);
    AddItem(" -Note", NOTE, logo_allArray[8], &Note_Item, &Home_Page, NULL, NULL);
    AddItem(" -Stock", GUPIAO, logo_allArray[9], &Gupiao_Item, &Home_Page, NULL, NULL);
    AddItem(" -Settings", SETTINGS, logo_allArray[10], &Settings_Item, &Home_Page, NULL, NULL);
}

void Menu_Init(xpMenu Menu)
{
    Disp_Init();
    Create_Menu(Menu, &HomeHead_Item);
    Application::Draw_Home(NULL);
}
