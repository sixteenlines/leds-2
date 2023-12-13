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

int initWifi();
bool clientSetup();
void managerSetup();
void hostManager();
void hostIndex();
void dnsNext();
void softReset();
bool handleFileRequest(AsyncWebServerRequest *request, String path);

#endif