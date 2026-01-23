// http.h - HTTP communication module header
#ifndef HTTP_H
#define HTTP_H

#include "config.h"

// Function declarations
bool setupServer();
void handleClient();
void sendToCEW();
void sendHeartbeat();
bool sendWithRetry(String url, String json);
void fetchWeather();

#endif // HTTP_H
