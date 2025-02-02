#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include <Button2.h>

// 按键引脚定义
#define BTN_LEFT_UP 22
#define BTN_RIGHT_DOWN 5

// 按键状态枚举
enum ButtonAction
{
    NONE = 0,       // 无动作
    LEFT_UP = 1,    // 向左/向上
    RIGHT_DOWN = 2, // 向右/向下
    CONFIRM = 3,    // 确认（双击）
};

// 声明函数
void initButtons();
ButtonAction getButtonAction();
void handleButtons(); // 处理按键事件

#endif