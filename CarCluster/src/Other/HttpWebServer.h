// SPDX-FileCopyrightText: 2026 WesVoj
// SPDX-License-Identifier: GPL-3.0-only

#ifndef HTTP_WEB_SERVER_H
#define HTTP_WEB_SERVER_H

#include "Arduino.h"

struct HttpString {
  const char *buf;
  size_t len;
};

struct HttpMessage {
  HttpString method;
  HttpString body;
};

struct HttpConnection {
  // Kept so the existing handlers can explicitly mark a response as complete.
  // ESP32 WebServer closes or reuses the connection itself.
  bool is_draining = false;
};

constexpr int HTTP_EVENT_REQUEST = 1;

using HttpEventHandler = void (*)(HttpConnection *, int, void *);

struct DashboardState {
  int speed;
  int maximumSpeed;
  int rpm;
  int maximumRPM;
  char gear[3];
  int fuel;
  int backlight;
  int coolant_temp;
  int maximumCoolantTemp;
  int minimumCoolantTemp;
  int outdoor_temp;
  bool high_beam;
  bool main_lights;
  bool left_indicator;
  bool right_indicator;
  bool fog_front;
  bool fog_rear;
  bool door_open;
  bool dsc;
  bool abs;
  bool handbrake;
  bool ignition;
  bool indicators_blink;
  char drive_mode[10];
};

using DashboardStateGetter = void (*)(DashboardState *);
using DashboardStateSetter = void (*)(DashboardState *);
using DashboardActionChecker = bool (*)();
using DashboardActionStarter = void (*)(HttpString);

void webServerSetHttpHandlers(const char *name, DashboardStateGetter getter, DashboardStateSetter setter);
void webServerSetHttpHandlers(const char *name, DashboardActionChecker checker, DashboardActionStarter starter);
void webServerAddRoute(const char *path, HttpEventHandler handler);
void webServerInit();
void webServerPoll();
void webServerNotifyStateChanged();

HttpString httpString(const char *value);
int httpStringCompare(HttpString left, HttpString right);
int httpGetVar(const HttpString *unusedBody, const char *name, char *destination, size_t destinationSize);
void httpReply(HttpConnection *connection, int statusCode, const char *headers, const char *format, ...);
void httpSendDownload(HttpConnection *connection, const char *filename, const char *data, size_t length);

#endif
