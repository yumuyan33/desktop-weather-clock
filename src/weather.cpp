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
#include "settings.h"

#include "weather.h"
#include "dispDirver.h"
#include "image.h"

// https://devapi.qweather.com/v7/weather/3d?location=101220101&key=7ea17a81fba242bf9aabca97d243a006

// const String url_3d = "https://devapi.qweather.com/v7/weather/3d"; // 接口地址：https://devapi.qweather.com/v7/weather/now  后面是now代表当前天气，改为3d代表未来3day天气
// const String key = "7ea17a81fba242bf9aabca97d243a006";						 // 自己注册一个账号获取key
// const String location = "101220101";															 // h合肥市 //填入城市编号  获取编号 https://where.heweather.com/index.html

// width=22, height=10
const unsigned char jintian[] = {
		0x20, 0xf0, 0x07, 0x50, 0x80, 0x00, 0x88, 0x80, 0x00, 0x26, 0xfb, 0x0f, 0x40, 0x80, 0x00, 0xf8, 0x40, 0x01, 0x80, 0x40, 0x01, 0x40, 0x20, 0x02, 0x30, 0x18, 0x0c, 0x00, 0x00, 0x00};

// width=22, height=10
const unsigned char mingtian[] = {
		0xc0, 0xf3, 0x07, 0x5e, 0x82, 0x00, 0x52, 0x82, 0x00, 0xd2, 0xfb, 0x0f, 0x5e, 0x82, 0x00, 0xd2, 0x43, 0x01, 0x5e, 0x42, 0x01, 0x20, 0x22, 0x02, 0x20, 0x1b, 0x0c, 0x00, 0x00, 0x00};

// width=22, height=10
const unsigned char houtian[] = {
		0xc0, 0xf0, 0x07, 0x3c, 0x80, 0x00, 0x04, 0x80, 0x00, 0xfc, 0xfb, 0x0f, 0x04, 0x80, 0x00, 0xf4, 0x41, 0x01, 0x14, 0x41, 0x01, 0x12, 0x21, 0x02, 0xf2, 0x19, 0x0c, 0x00, 0x00, 0x00};

// 定义天气数据结构
struct WeatherData
{
	String date;		// 日期
	String maxTemp; // 最高温度
	String minTemp; // 最低温度
	int iconDay;		// 天气图标代码
	String month;		// 月份
	String day;			// 日期
};

// 使用结构体数组替代多个全局变量
static WeatherData weatherData[3]; // 0:今天, 1:明天, 2:后天
static bool isFirstUpdate = true;
static unsigned long lastUpdate = 0;
static const unsigned long UPDATE_INTERVAL = 3600000; // 1小时更新一次

String weather_date_today;
String weather_maxtemp_today;
String weather_mintemp_today;

String weather_date_tomor; // tomorrow
String weather_maxtemp_tomor;
String weather_mintemp_tomor;

String weather_date_aftto;
String weather_maxtemp_aftto; // after tomorrow
String weather_mintemp_aftto;

String weather_month_today;
String weather_month_tomor;
String weather_month_aftto;

String weather_day_today;
String weather_day_tomor;
String weather_day_aftto;

int weather_icon_today;
int weather_icon_tomor;
int weather_icon_aftto;

String weather_date_now;
String weather_temp_now;
String weather_month_now;
String weather_day_now;

int weather_icon_now;
int size_of_temp[6] = {32, 32, 32, 32, 32, 32};

uint8_t *outbuf;

bool flag_get_weather = false;
const int CONFIG_HTTPPORT = 80;
const int CONFIG_HTTPSPORT = 443; // 在和风天气开发服务中，所有端口均为443

unsigned int localPort = 8266; /* 修改udp 有些路由器端口冲突时修改 */
int servoLift = 1500;

unsigned long current_time = 0;
unsigned long preserve_time = 0;
long one_hour = 3600000; // 60*60*1000

bool requestWeatherData(String url);
void parseJSON(char *input, int inputLength);

bool requestWeatherData(String url)
{
	if (WiFi.status() != WL_CONNECTED)
	{
		Serial.println("[Weather] WiFi未连接");
		return false;
	}

	HTTPClient http;
	String requestUrl = url + "?location=" + cityLocation + "&key=" + weatherKey;
	http.begin(requestUrl);

	Serial.println("[Weather] 开始请求天气数据...");
	int httpCode = http.GET();

	if (httpCode > 0)
	{
		if (httpCode == HTTP_CODE_OK)
		{
			Serial.printf("code=%d\n", httpCode);
			int len = -1;
			Serial.printf("size=%d\n", len);
			Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

			uint8_t buff[2048] = {0};
			WiFiClient *stream = http.getStreamPtr();
			Serial.println("stream ok");

			if (http.connected() && (len > 0 || len == -1))
			{
				size_t size = stream->available();
				Serial.printf("avail size=%d\n", size);

				if (size)
				{
					size_t readBytesSize = stream->readBytes(buff, size);
					if (len > 0)
					{
						len -= readBytesSize;
					}

					outbuf = (uint8_t *)malloc(sizeof(uint8_t) * 15000);
					uint32_t outprintsize = 0;
					int result = ArduinoZlib::libmpq__decompress_zlib(buff, readBytesSize, outbuf, 15000, outprintsize);

					parseJSON((char *)outbuf, outprintsize);
					free(outbuf);
					flag_get_weather = true;

					http.end();
					return true;
				}
			}
		}
	}
	else
	{
		flag_get_weather = false;
		Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
	}

	http.end();
	return false;
}

void parseJSON(char *input, int inputLength)
{
	DynamicJsonDocument doc(6144);
	DeserializationError error = deserializeJson(doc, input, inputLength);

	if (error)
	{
		Serial.print(F("deserializeJson() failed: "));
		Serial.println(error.f_str());
		return;
	}

	// 解析三天的数据
	JsonObject dailyDataToday = doc["daily"][0];
	JsonObject dailyDataTomor = doc["daily"][1];
	JsonObject dailyDataAftto = doc["daily"][2];

	weather_date_today = dailyDataToday["fxDate"].as<String>();
	weather_month_today = weather_date_today.substring(5, 7);
	weather_day_today = weather_date_today.substring(8, 10);
	weather_maxtemp_today = dailyDataToday["tempMax"].as<String>();
	weather_mintemp_today = dailyDataToday["tempMin"].as<String>();
	weather_icon_today = dailyDataToday["iconDay"].as<int>();

	weather_date_tomor = dailyDataTomor["fxDate"].as<String>();
	weather_month_tomor = weather_date_tomor.substring(5, 7);
	weather_day_tomor = weather_date_tomor.substring(8, 10);
	weather_maxtemp_tomor = dailyDataTomor["tempMax"].as<String>();
	weather_mintemp_tomor = dailyDataTomor["tempMin"].as<String>();
	weather_icon_tomor = dailyDataTomor["iconDay"].as<int>();

	weather_date_aftto = dailyDataAftto["fxDate"].as<String>();
	weather_month_aftto = weather_date_aftto.substring(5, 7);
	weather_day_aftto = weather_date_aftto.substring(8, 10);
	weather_maxtemp_aftto = dailyDataAftto["tempMax"].as<String>();
	weather_mintemp_aftto = dailyDataAftto["tempMin"].as<String>();
	weather_icon_aftto = dailyDataAftto["iconDay"].as<int>();

	weather_date_now = doc["now"]["obsTime"].as<String>();
	weather_month_now = weather_date_now.substring(5, 7);
	weather_day_now = weather_date_now.substring(8, 10);
	weather_temp_now = doc["now"]["temp"].as<String>();
	weather_icon_now = doc["now"]["icon"].as<int>();

	// 记录温度字符串长度
	size_of_temp[0] = weather_maxtemp_today.length();
	size_of_temp[1] = weather_mintemp_today.length();
	size_of_temp[2] = weather_maxtemp_tomor.length();
	size_of_temp[3] = weather_mintemp_tomor.length();
	size_of_temp[4] = weather_maxtemp_aftto.length();
	size_of_temp[5] = weather_mintemp_aftto.length();

	// 打印调试信息
	Serial.printf("size_of_temp[0] is %d\n", size_of_temp[0]);
	Serial.printf("size_of_temp[1] is %d\n", size_of_temp[1]);
	Serial.printf("size_of_temp[2] is %d\n", size_of_temp[2]);
	Serial.printf("size_of_temp[3] is %d\n", size_of_temp[3]);
	Serial.printf("size_of_temp[4] is %d\n", size_of_temp[4]);
	Serial.printf("size_of_temp[5] is %d\n", size_of_temp[5]);

	Serial.printf("weather_date_today is %s\r\n", weather_date_today);
	Serial.printf("weather_maxtemp_today is %s\r\n", weather_maxtemp_today);
	Serial.printf("weather_mintemp_today is %s\r\n", weather_mintemp_today);
	Serial.printf("weather_icon_today is %d\r\n", weather_icon_today);

	Serial.printf("weather_temp_now is %s\r\n", weather_temp_now);
	Serial.printf("weather_icon_now is %d\r\n", weather_icon_now);
}

// 绘制天气图标
void drawWeatherIcon(uint16_t x, uint16_t y, int iconCode)
{
	const uint8_t *icon = nullptr;

	switch (iconCode)
	{
	case 100:
	case 105:
		icon = iconDay_100;
		break;
	case 101:
	case 102:
	case 151:
	case 152:
		icon = iconDay_101;
		break;
	case 104:
		icon = iconDay_104;
		break;
	case 300:
	case 305:
	case 309:
	case 350:
		icon = iconDay_300;
		break;
	case 301:
	case 306:
	case 315:
	case 399:
		icon = iconDay_301;
		break;
	case 307:
	case 308:
	case 310:
	case 311:
	case 312:
	case 316:
	case 317:
	case 318:
	case 351:
		icon = iconDay_307;
		break;
	case 302:
	case 303:
		icon = iconDay_302;
		break;
	case 400:
	case 401:
	case 402:
	case 403:
	case 404:
	case 405:
	case 406:
	case 407:
	case 408:
	case 409:
	case 410:
	case 456:
	case 457:
	case 499:
		icon = iconDay_400;
		break;
	case 999:
		icon = iconDay_999;
		break;
	}

	if (icon)
	{
		OLED_DrawXBMP(x, y, 30, 30, icon);
	}
}

void Show_weather(xpMenu Menu)
{
	unsigned long currentTime = millis();

	// 检查是否配置了API密钥和城市编号
	if (weatherKey.length() == 0 || cityLocation.length() == 0)
	{
		OLED_ClearBuffer();
		OLED_SetFont(u8g2_font_wqy12_t_gb2312);
		OLED_DrawUTF8(20, 35, "请先配置API参数");
		OLED_SendBuffer();
		return;
	}

	// 首次运行或达到更新间隔时更新数据
	if (isFirstUpdate || (currentTime - lastUpdate >= UPDATE_INTERVAL))
	{
		// 显示加载提示
		OLED_ClearBuffer();
		OLED_SetFont(u8g2_font_wqy12_t_gb2312);
		OLED_DrawUTF8(20, 35, "正在加载天气...");
		OLED_SendBuffer();

		String url_3d = "https://devapi.qweather.com/v7/weather/3d"; // 移到函数内部
		requestWeatherData(url_3d);
		lastUpdate = currentTime;
		isFirstUpdate = false;
	}

	// 检查数据是否获取成功
	if (!flag_get_weather)
	{
		OLED_ClearBuffer();
		OLED_SetFont(u8g2_font_wqy12_t_gb2312);
		OLED_DrawUTF8(20, 35, "获取天气失败");
		OLED_SendBuffer();
		return;
	}

	OLED_ClearBuffer();

	// 定义布局常量
	const int COLUMN_WIDTH = 42;	// 每列宽度
	const int COLUMN_PADDING = 1; // 列间距
	const int TITLE_Y = 4;				// 标题Y坐标
	const int ICON_Y = 20;				// 图标Y坐标
	const int TEMP_Y = 62;				// 温度Y坐标
	const int SCREEN_WIDTH = 128; // 屏幕宽度

	// 计算第一列的起始X坐标，使三列居中
	const int START_X = (SCREEN_WIDTH - (COLUMN_WIDTH * 3 + COLUMN_PADDING * 2)) / 2;

	// 显示三列内容
	for (int i = 0; i < 3; i++)
	{
		int columnX = START_X + i * (COLUMN_WIDTH + COLUMN_PADDING);

		// 显示标题
		OLED_SetFont(MENU_FONT_CH);
		switch (i)
		{
		case 0:
			OLED_DrawXBMP(columnX + 8, TITLE_Y, 22, 10, jintian);
			break;
		case 1:
			OLED_DrawXBMP(columnX + 8, TITLE_Y, 22, 10, mingtian);
			break;
		case 2:
			OLED_DrawXBMP(columnX + 8, TITLE_Y, 22, 10, houtian);
			break;
		}

		// 显示天气图标
		const int ICON_SIZE = 30;
		int iconX = columnX + (COLUMN_WIDTH - ICON_SIZE) / 2;
		int iconCode;
		String maxTemp, minTemp;

		// 根据列选择对应数据
		switch (i)
		{
		case 0:
			iconCode = weather_icon_today;
			maxTemp = weather_maxtemp_today;
			minTemp = weather_mintemp_today;
			break;
		case 1:
			iconCode = weather_icon_tomor;
			maxTemp = weather_maxtemp_tomor;
			minTemp = weather_mintemp_tomor;
			break;
		case 2:
			iconCode = weather_icon_aftto;
			maxTemp = weather_maxtemp_aftto;
			minTemp = weather_mintemp_aftto;
			break;
		}

		// 绘制天气图标
		drawWeatherIcon(iconX, ICON_Y, iconCode);

		// 显示温度
		OLED_SetFont(MENU_FONT_victory);
		String tempStr = maxTemp + "/" + minTemp;
		int tempWidth = OLED_GetStrWidth(tempStr.c_str());
		int tempX = columnX + (COLUMN_WIDTH - tempWidth) / 2;

		OLED_DrawStr(tempX, TEMP_Y, maxTemp.c_str());
		int slashX = tempX + OLED_GetStrWidth(maxTemp.c_str());
		OLED_DrawStr(slashX, TEMP_Y, "/");
		int minTempX = slashX + OLED_GetStrWidth("/");
		OLED_DrawStr(minTempX, TEMP_Y, minTemp.c_str());
	}

	OLED_SendBuffer();

	// // 打印调试信息
	// Serial.println("[Weather] 显示更新完成");
	// Serial.printf("今天: %s/%s°C (图标: %d)\n",
	// 							weather_maxtemp_today.c_str(), weather_mintemp_today.c_str(), weather_icon_today);
	// Serial.printf("明天: %s/%s°C (图标: %d)\n",
	// 							weather_maxtemp_tomor.c_str(), weather_mintemp_tomor.c_str(), weather_icon_tomor);
	// Serial.printf("后天: %s/%s°C (图标: %d)\n",
	// 							weather_maxtemp_aftto.c_str(), weather_mintemp_aftto.c_str(), weather_icon_aftto);
}

void getWeather()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		return;
	}

	HTTPClient http;
	String url = "https://devapi.qweather.com/v7/weather/now";
	http.begin(url + "?location=" + cityLocation + "&key=" + weatherKey);

	Serial.println("[Weather] 开始请求天气数据...");
	int httpCode = http.GET();

	if (httpCode > 0)
	{
		if (httpCode == HTTP_CODE_OK)
		{
			Serial.printf("code=%d\n", httpCode);
			int len = -1;
			Serial.printf("size=%d\n", len);
			Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

			uint8_t buff[2048] = {0};
			WiFiClient *stream = http.getStreamPtr();
			Serial.println("stream ok");

			if (http.connected() && (len > 0 || len == -1))
			{
				size_t size = stream->available();
				Serial.printf("avail size=%d\n", size);

				if (size)
				{
					size_t readBytesSize = stream->readBytes(buff, size);
					if (len > 0)
					{
						len -= readBytesSize;
					}

					outbuf = (uint8_t *)malloc(sizeof(uint8_t) * 15000);
					uint32_t outprintsize = 0;
					int result = ArduinoZlib::libmpq__decompress_zlib(buff, readBytesSize, outbuf, 15000, outprintsize);

					parseJSON((char *)outbuf, outprintsize);
					free(outbuf);
					flag_get_weather = true;

					http.end();
					return;
				}
			}
		}
	}
	else
	{
		flag_get_weather = false;
		Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
	}

	http.end();
}
