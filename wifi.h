  #ifndef WIFI_H
  #define WIFI_H

  #include <WiFi.h>
  #include <WebServer.h>  // Built-in ESP32 library

  WebServer *server = nullptr;
  bool wifiAvailable = false;

  #endif
