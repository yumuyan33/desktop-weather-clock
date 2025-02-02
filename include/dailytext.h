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


#include "dispDirver.h"
#include "image.h"

void Show_dailytext(xpMenu Menu);