#include <Arduino.h>
#include "picture.h"
#include <U8G2lib.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <string.h>
#include "dispDirver.h"
#include "heart.h"
#include "stdio.h"
#include "button.h"
#include "menu.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C U8G2(U8G2_R0, 16, 15, 2);

static bool isNormalExit = false;

void ground_run();
void mariao_action(xpMenu Menu);
void cloud_run();
void big_cloud_run();
void grass_run(int offset_x, float speed);
void game_run(xpMenu Menu);
void chimney_run(int offset_x, float speed);
void blank_run(int offset_x, float speed);
void game_restart(xpMenu Menu);
void game_over();
void serialEventRun();
void coin_run();
extern void Menu_Loop(xpMenu Menu);

static int pos_y_mariao = 40;
#define pos_x_mariao 14
#define width_mariao 8
#define height_mariao 12

static int pos_x_chimney = 128;
#define pos_y_chimney 40
#define width_chimney 12
#define height_chimney 12

static int pos_x_blank = 300;
#define pos_y_blank 52
#define width_blank 12
#define height_blank 12

static int pos_x_coin;
static int pos_y_coin;
#define width_coin 6
#define height_coin 8

bool Fail = false;
bool GetCoin = false;
unsigned int score = 0;
String DisScore;

static bool isJumping = false;

void game_setup(xpMenu Menu)
{
	// 显示开场动画
	OLED_ClearBuffer();
	Serial.printf("start....\r\n");
	U8G2.drawXBMP(0, 0, 128, 64, COVER1);
	OLED_SendBuffer();
	delay(500);
	U8G2.drawXBMP(0, 0, 128, 64, COVER2);
	OLED_SendBuffer();

	// 等待玩家按下键开始
	Menu->dir = MENU_NONE;
	while (Menu->dir != MENU_DOWN)
	{
		handleButtons();
		ButtonAction action = getButtonAction();
		if (action == CONFIRM)
		{
			Menu->dir = MENU_ENTER;
			isNormalExit = true;
			Change_MenuState(Menu, APP_QUIT);
			return;
		}
		if (action == RIGHT_DOWN)
		{
			Menu->dir = MENU_DOWN;
		}
		delay(10);
	}
	Menu->dir = MENU_NONE;
	// 按下后显示GAME START
	OLED_ClearBuffer();
	uint8_t color = 0;
	OLED_SetDrawColor(&color);
	OLED_DrawBox(20, 16, 88, 30);
	color = 1;
	OLED_SetDrawColor(&color);
	OLED_DrawFrame(19, 15, 90, 32);
	OLED_SetFont(u8g2_font_wqy14_t_gb2312a);
	OLED_DrawStr(26, 36, "GAME START");
	OLED_SendBuffer();

	// 给玩家一点时间看到GAME START
	delay(1000);
}

void Play_game(xpMenu Menu)
{
	game_setup(Menu);
	if (isNormalExit)
	{
		isNormalExit = false;
		return;
	}

	// 重置游戏状态
	Fail = false;
	GetCoin = false;
	score = 0;
	pos_x_blank = 300;
	pos_y_mariao = 40;
	pos_x_chimney = 128;
	pos_x_coin = 160;
	pos_y_coin = 30;

	while (true)
	{
		game_run(Menu);

		// 碰撞检测
		if ((((pos_x_mariao > pos_x_chimney) && (pos_x_mariao < (pos_x_chimney + width_chimney))) &&
				 ((pos_y_mariao + 10) > pos_y_chimney && (pos_y_mariao + 10) < (pos_y_chimney + height_chimney))) ||
				((pos_x_mariao > pos_x_blank && pos_x_mariao < pos_x_blank + width_blank) && (pos_y_mariao + height_mariao == pos_y_blank)))
		{
			Fail = true;
		}

		// 金币检测和得分逻辑
		if (((pos_x_coin >= pos_x_mariao - width_coin) && (pos_x_coin <= pos_x_mariao + width_mariao)) &&
				((pos_y_coin >= pos_y_mariao - height_coin) && (pos_y_coin <= pos_y_mariao + height_mariao)))
		{
			GetCoin = true;
		}

		if (GetCoin == true && (pos_x_coin == pos_x_mariao))
		{
			(score++) % 1000;
		}

		// 显示分数
		DisScore = String(score);
		OLED_SetFont(u8g2_font_6x10_mf);
		OLED_DrawStr(64, 12, "score  ");
		OLED_DrawStr(112, 12, DisScore.c_str());
		OLED_SendBuffer();

		if (Fail == true)
		{
			game_over();
			score = 0;

			// 等待玩家按键
			while (true)
			{
				handleButtons();
				ButtonAction action = getButtonAction();

				if (action == CONFIRM)
				{
					Change_MenuState(Menu, APP_QUIT);
					return;
				}
				else if (action == RIGHT_DOWN)
				{
					// 重置游戏状态
					Fail = false;
					GetCoin = false;
					pos_x_blank = 300;
					pos_y_mariao = 40;
					pos_x_chimney = 128;
					pos_x_coin = 160;
					pos_y_coin = 30;
					break; // 直接跳出内部循环，继续游戏主循环
				}

				delay(10);
			}
		}
	}
}

void game_run(xpMenu Menu)
{
	OLED_ClearBuffer();
	ground_run();
	cloud_run();
	coin_run();
	big_cloud_run();
	grass_run(rand() % 20, 1 + (rand() % 10 / 100));
	chimney_run(19, 1.04);
	blank_run(0, 1.04);
	mariao_action(Menu);
	delay(40);
}

void game_restart(xpMenu Menu)
{
	pos_x_blank = 300;
	pos_y_mariao = 40;
	pos_x_chimney = 128;
	pos_x_coin = 160;
	pos_y_coin = 30;

	OLED_ClearBuffer();

	Menu->dir = MENU_NONE;
	while (true)
	{
		handleButtons();
		ButtonAction action = getButtonAction();
		if (action == RIGHT_DOWN)
		{
			Menu->dir = MENU_DOWN;
			break;
		}
		delay(10);
	}
}

void ground_run()
{
	static int pos_x = 0;

	for (int i = 0; i < 23; i++)
	{
		OLED_DrawXBMP(6 * i - pos_x, 52, 6, 12, Ground);
	}

	if (pos_x > 5)
	{
		pos_x = 0;
	}
	else
	{
		pos_x++;
	}
}

void mariao_action(xpMenu Menu)
{
	static byte i = 0;
	static int offset_y = 0;
	static unsigned int dir_up = 1;
	pos_y_mariao = 40 - offset_y;

	// 直接检测按键状态
	handleButtons();
	ButtonAction action = getButtonAction();
	if (action == RIGHT_DOWN)
	{
		isJumping = true;
	}

	if (isJumping)
	{
		OLED_DrawXBMP(pos_x_mariao, pos_y_mariao, 6, 12, Mariao[0]);

		if (offset_y == 20)
		{
			dir_up = !dir_up;
		}

		if (dir_up == 1)
		{
			offset_y = offset_y + 2;
		}
		else
		{
			if (offset_y >= 0)
				offset_y = offset_y - 2;
			else
			{
				dir_up = !dir_up;
				offset_y = 0;
				isJumping = false;
			}
		}
	}
	else
	{
		OLED_DrawXBMP(pos_x_mariao, pos_y_mariao, 6, 12, Mariao[i == 0 || i == 1]);
	}

	if (i > 3)
	{
		i = 0;
	}
	else
	{
		i++;
	}
}

void cloud_run()
{
	static float pos_x = 128.0;
	static int pos_y = 12;
	OLED_DrawXBMP((int)pos_x, pos_y, 12, 8, Cloud);

	if (pos_x > -12)
	{
		pos_x = (int)(pos_x - 1.01f);
	}
	else
	{
		pos_x = 128 + rand() % 2;
		pos_y = 12 + rand() % 6;
	}
}

void coin_run()
{
	static int pos_x = 160;
	static int pos_y = 30;
	pos_x_coin = pos_x;
	pos_y_coin = pos_y;

	if (GetCoin == true)
	{
		OLED_DrawXBMP((int)pos_x_coin, pos_y_coin, 6, 8, CoinZero);
	}
	else
	{
		OLED_DrawXBMP(pos_x_coin, pos_y_coin, 6, 8, Coin);
	}

	if (pos_x > -12)
	{
		pos_x = (pos_x - 2);
	}
	else
	{
		GetCoin = false;
		pos_x = 128 + rand() % 20 * 10;
		pos_y = 30 + rand() % 6;
	}
}

void big_cloud_run()
{
	static int pos_x = 90;
	static int pos_y = 8;
	OLED_DrawXBMP(pos_x, pos_y, 16, 8, Big_Cloud);

	if (pos_x > -12)
	{
		pos_x--;
	}
	else
	{
		pos_x = 128 + rand() % 20;
		pos_y = 8 + rand() % 6;
	}
}

void grass_run(int offset_x, float speed)
{
	static float pos_x = 90;
	OLED_DrawXBMP(pos_x, 44, 16, 8, Grass);

	if (pos_x > -12)
	{
		pos_x = pos_x - speed;
	}
	else
	{
		pos_x = 128 + rand() % 20 + offset_x;
	}
}

void chimney_run(int offset_x, float speed)
{
	OLED_DrawXBMP(pos_x_chimney, pos_y_chimney, 12, 12, Chimney);

	if (pos_x_chimney > -12)
	{
		pos_x_chimney = pos_x_chimney - speed;
	}
	else
	{
		pos_x_chimney = 128 + rand() % 10 * 13 + offset_x;
	}
}

void blank_run(int offset_x, float speed)
{
	OLED_DrawXBMP(pos_x_blank, pos_y_blank, 12, 12, BLANK);

	if (pos_x_blank > -12)
	{
		pos_x_blank = pos_x_blank - speed;
	}
	else
	{
		pos_x_blank = 128 + pos_x_chimney + offset_x;
	}
}

void game_over()
{
	OLED_ClearBuffer();
	OLED_DrawXBMP(0, 0, 128, 64, RESTART);
	OLED_SetFont(u8g2_font_10x20_me);
	OLED_DrawStr(2, 20, "GAME");
	OLED_DrawStr(86, 20, "OVER");
	OLED_SendBuffer();
	Fail = false;
}
