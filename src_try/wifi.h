#ifndef WIFI_H
#define WIFI_H

#include <WebServer.h>  // Built-in ESP32 library
#include <WiFi.h>

WebServer* server        = nullptr;
bool       wifiAvailable = false;

#endif
