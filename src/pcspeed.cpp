#include "pcspeed.h"

// 存储CPU和内存使用率
static int cpuUsage = 0;
static int memUsage = 0;

// 解析串口数据
void parseSerialData(String data)
{
  // 检查数据格式是否正确
  if (!data.startsWith("wrrg="))
  {
    return;
  }

  // 分离数据类型和值
  int commaIndex = data.indexOf(',');
  if (commaIndex == -1)
  {
    return;
  }

  // 获取数据类型和值
  int dataType = data.substring(5, commaIndex).toInt();
  int value = data.substring(commaIndex + 1).toInt();

  // 根据数据类型存储值
  switch (dataType)
  {
  case 1: // CPU使用率
    cpuUsage = value;
    Serial.printf("[PCSpeed] CPU使用率: %d%%\n", cpuUsage);
    break;
  case 2: // 内存使用率
    memUsage = value;
    Serial.printf("[PCSpeed] 内存使用率: %d%%\n", memUsage);
    break;
  default:
    Serial.println("[PCSpeed] 未知数据类型");
    break;
  }
}

// 处理串口数据
void processSerialData()
{
  while (Serial.available())
  {
    String data = Serial.readStringUntil('\n');
    data.trim(); // 移除首尾空白字符

    if (data.length() > 0)
    {
      Serial.println("[PCSpeed] 收到数据: " + data);
      parseSerialData(data);
    }
  }
}

// 获取CPU使用率
int getCPUUsage()
{
  return cpuUsage;
}

// 获取内存使用率
int getMemUsage()
{
  return memUsage;
}

void Show_pcspeed(xpMenu Menu)
{
  // 检查并处理串口数据
  processSerialData();

  // 清屏并设置颜色
  OLED_ClearBuffer();
  OLED_SetDrawColor(&Menu->bgColor);
  OLED_DrawBox(0, 0, HOR_RES, VER_RES);
  uint8_t color = Menu->bgColor ^ 0x01;
  OLED_SetDrawColor(&color);

  // 定义布局参数
  const int LEFT_MARGIN = 5;   // 左边距
  const int TEXT_WIDTH = 35;   // CPU/RAM文字占用宽度
  const int RIGHT_MARGIN = 35; // 右边数字区域宽度
  const int BAR_HEIGHT = 12;   // 进度条高度
  const int VERTICAL_GAP = 10; // 上下两个指标的间距

  // 计算垂直位置
  const int CPU_Y = 30;                                // CPU区域的Y坐标
  const int RAM_Y = CPU_Y + BAR_HEIGHT + VERTICAL_GAP; // RAM区域的Y坐标

  // 计算进度条参数
  const int BAR_X = LEFT_MARGIN + TEXT_WIDTH;           // 进度条起始X坐标
  const int BAR_WIDTH = HOR_RES - BAR_X - RIGHT_MARGIN; // 进度条宽度

  // 设置字体
  OLED_SetFont(u8g2_font_7x14B_tr); // 使用粗体字体使文字更清晰

  // 绘制CPU部分
  OLED_DrawStr(LEFT_MARGIN, CPU_Y, "CPU");
  // 绘制CPU进度条外框
  OLED_DrawFrame(BAR_X, CPU_Y - BAR_HEIGHT + 2, BAR_WIDTH, BAR_HEIGHT);
  // 绘制CPU进度条填充
  int cpuBarFill = (BAR_WIDTH - 2) * cpuUsage / 100;
  OLED_DrawBox(BAR_X + 1, CPU_Y - BAR_HEIGHT + 3, cpuBarFill, BAR_HEIGHT - 2);
  // 绘制CPU百分比
  char str[5];
  sprintf(str, "%d%%", cpuUsage);
  OLED_DrawStr(BAR_X + BAR_WIDTH + 5, CPU_Y, str);

  // 绘制RAM部分
  OLED_DrawStr(LEFT_MARGIN, RAM_Y, "RAM");
  // 绘制RAM进度条外框
  OLED_DrawFrame(BAR_X, RAM_Y - BAR_HEIGHT + 2, BAR_WIDTH, BAR_HEIGHT);
  // 绘制RAM进度条填充
  int ramBarFill = (BAR_WIDTH - 2) * memUsage / 100;
  OLED_DrawBox(BAR_X + 1, RAM_Y - BAR_HEIGHT + 3, ramBarFill, BAR_HEIGHT - 2);
  // 绘制RAM百分比
  sprintf(str, "%d%%", memUsage);
  OLED_DrawStr(BAR_X + BAR_WIDTH + 5, RAM_Y, str);

  OLED_SendBuffer();
}
