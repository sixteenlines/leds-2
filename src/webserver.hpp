#ifndef _ARDUINO_H
#define _ARDUINO_H
#include <Arduino.h>
#endif

#ifndef _WS_HPP
#define _WS_HPP

#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

bool initTime();
int initWifi();
bool clientSetup();
void managerSetup();
void hostManager();
void hostIndex();
void dnsNext();
bool handleFileRequest(AsyncWebServerRequest *request, String path);
void checkDayOrNight(struct tm *localTime, struct tm *sunrise_tm, struct tm *sunset_tm);
bool getSunsetSunrise(struct tm *sunrise_tm, struct tm *sunset_tm);
int timeToSecondsSinceMidnight(struct tm *timeinfo);
bool convertToTM(const char *timeStr, struct tm *timeinfo);

#endif