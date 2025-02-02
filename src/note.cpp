#include "note.h"

// 存储最多4条便签
#define MAX_NOTES 4
#define MAX_NOTE_LENGTH 128 // 每条便签最大字符长度

// 便签存储数组
static String notes[MAX_NOTES];

// 解析便签数据
void parseNoteData(String data)
{
  // 检查数据格式是否正确
  if (!data.startsWith("note="))
  {
    return;
  }

  // 分离便签编号和内容
  int commaIndex = data.indexOf(',');
  if (commaIndex == -1)
  {
    return;
  }

  // 获取便签编号（1-4）
  int noteIndex = data.substring(5, commaIndex).toInt() - 1;
  if (noteIndex < 0 || noteIndex >= MAX_NOTES)
  {
    Serial.println("[Note] 无效的便签编号");
    return;
  }

  // 获取便签内容
  String noteContent = data.substring(commaIndex + 1);

  // 存储便签
  notes[noteIndex] = noteContent;

  Serial.printf("[Note] 便签 %d 已更新: %s\n", noteIndex + 1, noteContent.c_str());
}

// 处理串口数据
void processNoteData()
{
  while (Serial.available())
  {
    String data = Serial.readStringUntil('\n');
    data.trim(); // 移除首尾空白字符

    if (data.length() > 0)
    {
      Serial.println("[Note] 收到数据: " + data);
      parseNoteData(data);
    }
  }
}

// 获取指定编号的便签内容
String getNoteContent(int index)
{
  if (index >= 0 && index < MAX_NOTES)
  {
    return notes[index];
  }
  return "";
}

void take_note(xpMenu Menu)
{
  // 检查并处理串口数据
  processNoteData();

  // 清屏并设置颜色
  OLED_ClearBuffer();
  OLED_SetDrawColor(&Menu->bgColor);
  OLED_DrawBox(0, 0, HOR_RES, VER_RES);
  uint8_t color = Menu->bgColor ^ 0x01;
  OLED_SetDrawColor(&color);

  // 定义布局参数
  const int TITLE_Y = 12;         // 标题Y坐标
  const int LINE_Y = TITLE_Y + 4; // 分割线Y坐标
  const int FIRST_NOTE_Y = 28;    // 第一条便签Y坐标
  const int NOTE_SPACING = 12;    // 便签之间的间距
  const int DOT_RADIUS = 2;       // 圆点半径
  const int DOT_MARGIN = 8;       // 圆点左边距
  const int TEXT_MARGIN = 16;     // 文字左边距

  // 设置标题字体
  OLED_SetFont(u8g2_font_wqy12_t_gb2312); // 使用中文字体

  // 绘制标题
  OLED_DrawUTF8(2, TITLE_Y, "待办事项");

  // 绘制分割线
  OLED_DrawHLine(0, LINE_Y, HOR_RES);

  // 设置便签字体
  OLED_SetFont(u8g2_font_wqy12_t_gb2312); // 使用中文字体显示便签内容

  // 绘制便签
  for (int i = 0; i < MAX_NOTES; i++)
  {
    int noteY = FIRST_NOTE_Y + i * NOTE_SPACING;

    // 绘制圆点
    OLED_DrawDisc(DOT_MARGIN, noteY - 3, DOT_RADIUS);

    // 绘制便签内容
    if (notes[i].length() > 0)
    {
      OLED_DrawUTF8(TEXT_MARGIN, noteY, notes[i].c_str());
    }
  }

  OLED_SendBuffer();
}