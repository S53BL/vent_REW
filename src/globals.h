// globals.h - Global variables and settings
#ifndef GLOBALS_H
#define GLOBALS_H

#include "config.h"
#include <ezTime.h>

// Extern declarations
extern SensorData sensorData;
extern Settings settings;
extern Timezone myTZ;
extern uint32_t lastSensorRead;
extern uint32_t lastHttpSend;
extern uint32_t lastHeartbeat;
extern uint32_t lastWeatherUpdate;
extern uint32_t lastArchive;
extern uint32_t lastSensorReset;
extern unsigned long lastSEWReceive;
extern unsigned long lastStatusUpdate;
extern bool webServerRunning;
extern bool timeSynced;
extern String wifiSSID;

// Function declarations
void initGlobals();
void loadSettings();
void saveSettings();

extern String currentSensFile;
extern String currentFanFile;
extern String currentLogFile;
extern String lastDate;
extern String logBuffer;
extern uint32_t lastHistorySave;

extern bool is_pressed;
extern unsigned long touch_press_time;

#endif // GLOBALS_H
