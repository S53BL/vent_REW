// web.h

#ifndef WEB_H
#define WEB_H

#include <ESPAsyncWebServer.h>
#include "config.h"

void setupWebServer();

extern bool sendingHTTP;
extern int httpStatus;
extern unsigned long httpStatusTime;
extern bool webServerRunning;
extern const Settings defaultSettings;
extern SensorData sensorData;

#endif // WEB_H
// web.h