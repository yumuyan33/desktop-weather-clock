#include "dispDirver.h"
#include "heart.h"
#include "stdio.h"

#define BASELINE_Y 30  // 基准线Y坐标
#define WAVE_HEIGHT 20 // 波形高度
#define SCAN_STEP 8    // 扫描步进
#define MAX_RANDOM 2   // 减小最大随机偏移值

// 心形图标数据 (12x12像素)
static const uint8_t heart_icon[] = {
    0x0C, 0x30,
    0x1E, 0x78,
    0x3F, 0xFC,
    0x7F, 0xFE,
    0x7F, 0xFE,
    0x7F, 0xFE,
    0x3F, 0xFC,
    0x1F, 0xF8,
    0x0F, 0xF0,
    0x07, 0xE0,
    0x03, 0xC0,
    0x01, 0x80};

static int scan_position = 0;
static int random_offset = 0;
static int heart_value = 0;         // 用于显示的递增数值
static bool wave_completed = false; // 标记波形是否完成一次扫描

// 调整波形关键点坐标，减小波峰高度
static const int WAVE_POINTS[][2] = {
    {0, BASELINE_Y},       // 起点
    {8, BASELINE_Y - 5},   // 第一个波峰
    {16, BASELINE_Y + 8},  // 第二个波峰
    {24, BASELINE_Y - 20}, // 主波峰
    {32, BASELINE_Y + 2},  // 回落点
    {40, BASELINE_Y},      // 结束点
    {128, BASELINE_Y}      // 基线终点
};

void Heart_beat(xpMenu Menu)
{
    // 获取当前值
    float value = 0;
    switch (Menu->now_item->element->data->Data_Type)
    {
    case DATA_INT:
        value = (float)(*(int *)(Menu->now_item->element->data->ptr));
        break;
    case DATA_FLOAT:
        value = *(float *)(Menu->now_item->element->data->ptr);
        break;
    default:
        break;
    }

    // 清屏并设置颜色
    OLED_ClearBuffer();
    OLED_SetDrawColor(&Menu->bgColor);
    OLED_DrawBox(0, 0, HOR_RES, VER_RES);
    uint8_t color = Menu->bgColor ^ 0x01;
    OLED_SetDrawColor(&color);

    // 绘制波形
    if (scan_position < HOR_RES + WAVE_POINTS[5][0])
    {
        wave_completed = false; // 重置完成标志

        // 绘制左侧基线
        int baselineStart = 0;
        int waveStart = scan_position - WAVE_POINTS[5][0];
        if (waveStart > 0)
        {
            OLED_DrawLine(0, BASELINE_Y, waveStart, BASELINE_Y);
            baselineStart = waveStart;
        }

        // 计算所有点的偏移值
        int offsets[7] = {0};       // 为每个点存储偏移值
        for (int i = 1; i < 5; i++) // 只对中间的点应用偏移
        {
            int maxOffset = (i == 2) ? 1 : MAX_RANDOM;
            offsets[i] = (random_offset % (maxOffset * 2 + 1)) - maxOffset;
        }

        // 绘制波形线段
        int lastX = -1, lastY = -1;
        for (int i = 0; i < 6; i++)
        {
            int x1 = WAVE_POINTS[i][0] + scan_position - WAVE_POINTS[5][0];
            int y1 = WAVE_POINTS[i][1] + offsets[i];
            int x2 = WAVE_POINTS[i + 1][0] + scan_position - WAVE_POINTS[5][0];
            int y2 = WAVE_POINTS[i + 1][1] + offsets[i + 1];

            // 确保与上一个线段的终点相连
            if (lastX != -1 && lastY != -1 && x1 == lastX)
            {
                y1 = lastY;
            }

            // 只绘制在屏幕范围内的线段
            if (x1 >= 0 && x1 < HOR_RES && x2 >= 0 && x2 < HOR_RES)
            {
                OLED_DrawLine(x1, y1, x2, y2);
                lastX = x2;
                lastY = y2;
            }
        }

        // 绘制右侧基线
        int waveEnd = scan_position - WAVE_POINTS[5][0] + WAVE_POINTS[5][0];
        if (waveEnd < HOR_RES)
        {
            // 确保与最后一个波形点平滑连接
            if (lastX != -1 && lastY != -1 && waveEnd == lastX)
            {
                OLED_DrawLine(lastX, lastY, HOR_RES, BASELINE_Y);
            }
            else
            {
                OLED_DrawLine(waveEnd, BASELINE_Y, HOR_RES, BASELINE_Y);
            }
        }

        scan_position++;
    }
    else
    {
        if (!wave_completed) // 只在波形首次完成时更新计数
        {
            heart_value = (heart_value + 1) % 1000; // 在0-999之间循环
            wave_completed = true;                  // 标记本次波形已完成
        }
        scan_position = 0;
        random_offset = esp_random();
    }

    // 在最后更新显示之前，绘制心形图标和数值
    // 绘制心形图标
    OLED_SetFont(u8g2_font_siji_t_6x10);
    OLED_DrawGlyph(6, VER_RES - 1, 0xe107);

    // 显示递增数值
    char str[5];
    sprintf(str, "%d", heart_value);
    OLED_SetFont(u8g2_font_6x12_tr); // 使用小号字体
    OLED_DrawStr(24, VER_RES - 2, str);

    OLED_SendBuffer();
}