#include "button.h"

// 创建按键对象
Button2 btnLeftUp;
Button2 btnRightDown;

// 用于存储最近的按键动作
static ButtonAction lastAction = NONE;

// 初始化按键
void initButtons()
{
  // 配置按键
  btnLeftUp.begin(BTN_LEFT_UP, INPUT_PULLUP);
  btnRightDown.begin(BTN_RIGHT_DOWN, INPUT_PULLUP);

  // 设置按键时间参数
  btnLeftUp.setLongClickTime(800); // 设置长按时间为800ms
  btnLeftUp.setDebounceTime(50);   // 消抖时间

  btnRightDown.setLongClickTime(800); // 设置长按时间为800ms
  btnRightDown.setDebounceTime(50);   // 消抖时间

  // 左/上按键事件处理
  btnLeftUp.setClickHandler([](Button2 &btn)
                            { 
                              Serial.println("Left/Up Click");
                              lastAction = LEFT_UP; });

  btnLeftUp.setLongClickDetectedHandler([](Button2 &btn)
                                        { 
                                         Serial.println("Left/Up Long Press Detected - Confirm");
                                         lastAction = CONFIRM; });

  // 右/下按键事件处理
  btnRightDown.setClickHandler([](Button2 &btn)
                               { 
                                 Serial.println("Right/Down Click");
                                 lastAction = RIGHT_DOWN; });

  btnRightDown.setLongClickDetectedHandler([](Button2 &btn)
                                           { 
                                            Serial.println("Right/Down Long Press Detected - Confirm");
                                            lastAction = CONFIRM; });
}

// 处理按键事件
void handleButtons()
{
  btnLeftUp.loop();
  btnRightDown.loop();
}

// 获取按键动作
ButtonAction getButtonAction()
{
  ButtonAction action = lastAction;
  lastAction = NONE; // 清除动作状态
  return action;
}
