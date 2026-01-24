// http.h - HTTP communication module header
#ifndef HTTP_H
#define HTTP_H

#include "config.h"

// Function declarations
bool setupServer();
void handleClient();
void sendToCEW();
bool sendHeartbeat();
bool sendWithRetry(String url, String json);
void fetchWeather();
void sendManualControl(String room, String action);

#endif // HTTP_H
