// http.h - HTTP communication module header
#ifndef HTTP_H
#define HTTP_H

#include "config.h"

// Function declarations
bool setupServer();
void handleClient();
void sendToCEW(String method, String endpoint, String jsonPayload);
bool sendHeartbeat();
void fetchWeather();

#endif // HTTP_H
