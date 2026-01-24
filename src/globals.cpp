// globals.cpp - Global variables definitions and EEPROM handling
#include "globals.h"
#include <EEPROM.h>

// NTP servers definition
const char* ntpServers[] = {"pool.ntp.org", "time.nist.gov", "time.google.com"};

// Global variables definitions
SensorData sensorData;
Settings settings;
Timezone myTZ;
uint32_t lastSensorRead = 0;
uint32_t lastHttpSend = 0;
uint32_t lastHeartbeat = 0;
uint32_t lastWeatherUpdate = 0;
uint32_t lastSensorReset = 0;
uint32_t lastMinuteUpdate = 0;
unsigned long lastSEWReceive = 0;
unsigned long lastStatusUpdate = 0;
unsigned long lastSuccessfulHeartbeat = 0;
bool connection_ok = false;
bool webServerRunning = false;
bool timeSynced = false;
String wifiSSID = "";

String currentSensFile = "";
String currentFanFile = "";
String currentLogFile = "";
String lastDate = "";
String logBuffer = "";
uint32_t lastHistorySave = 0;
bool loggingInitialized = false;

bool is_pressed = false;
unsigned long touch_press_time = 0;
unsigned long touch_release_time = 0;
unsigned long last_touch_time = 0;
uint16_t last_touch_x = 0;
uint16_t last_touch_y = 0;
bool active_touch = false;
int pressed_count = 0;
int released_count = 0;

void initGlobals() {
    // Initialize sensorData with defaults
    sensorData.localTemp = 0.0;
    sensorData.localHumidity = 0.0;
    sensorData.localCO2 = 0.0;
    sensorData.localLux = 0.0;
    sensorData.extTemp = 0.0;
    sensorData.extHumidity = 0.0;
    sensorData.extPressure = 0.0;
    sensorData.extVOC = 0.0;
    sensorData.extLux = 0.0f;
    sensorData.lastTemp = NAN;
    sensorData.lastHumidity = NAN;
    sensorData.lastCO2 = NAN;
    memset(sensorData.errorFlags, 0, sizeof(sensorData.errorFlags));
    memset(sensorData.fans, 0, sizeof(sensorData.fans));
    memset(sensorData.fanStates, 0, sizeof(sensorData.fanStates));
    memset(sensorData.inputs, 0, sizeof(sensorData.inputs));
    sensorData.bathroomTemp = 0.0f;
    sensorData.bathroomHumidity = 0.0f;
    sensorData.bathroomPressure = 0.0f;
    sensorData.utTemp = 0.0f;
    sensorData.utHumidity = 0.0f;
    memset(sensorData.offTimes, 0, sizeof(sensorData.offTimes));
    sensorData.bathroomFan = false;
    sensorData.utilityFan = false;
    sensorData.dsFan = false;
    sensorData.bathroomLight = false;
    sensorData.utilityLight = false;
    sensorData.skylightOpen = false;
    sensorData.balconyDoorOpen = false;
    sensorData.bathroomFanStart = 0;
    sensorData.utilityFanStart = 0;
    sensorData.dsFanStart = 0;
    sensorData.currentPower = 0.0;
    sensorData.energyConsumption = 0.0;
    sensorData.weatherIcon = "sun.png";
    sensorData.weatherCode = 0;

    // Initialize settings with defaults
    settings.humThreshold = HUM_THRESHOLD;
    settings.fanDuration = 1800000;  // 30 minutes
    settings.tempMinThreshold = 18;
    settings.dndAllowAutomatic = true;
    memset(settings.nndDays, 0, sizeof(settings.nndDays));

    is_pressed = false;
    touch_press_time = 0;
    touch_release_time = 0;
    last_touch_time = 0;
    last_touch_x = 0;
    last_touch_y = 0;
    active_touch = false;
    pressed_count = 0;
    released_count = 0;

    Serial.println("Globals init OK");
}

void loadSettings() {
    EEPROM.begin(512);
    uint8_t marker;
    EEPROM.get(0, marker);
    if (marker == EEPROM_MARKER) {
        EEPROM.get(1, settings);
    } else {
        // First run, save defaults
        saveSettings();
    }
    EEPROM.end();
}

void saveSettings() {
    EEPROM.begin(512);
    EEPROM.put(0, EEPROM_MARKER);
    EEPROM.put(1, settings);
    EEPROM.commit();
    EEPROM.end();
}
