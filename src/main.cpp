// main.cpp - Main entry point for vent_REW
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <ezTime.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include "config.h"
#include "wifi_config.h"
#include "globals.h"
#include "sens.h"
#include "disp.h"
#include "http.h"
#include "sd.h"
#include <Touch_CST328.h>

TwoWire WireTouch = TwoWire(TOUCH_I2C_BUS);

// Touch buffer
uint16_t last_touch_x = 0;
uint16_t last_touch_y = 0;
uint16_t touch_strength = 0;
uint8_t point_num = 0;
uint8_t max_point_num = 1;
unsigned long last_touch_time = 0;

bool setupWiFi() {
    WiFi.config(localIP, gateway, subnet, dns);
    for (int i = 0; i < sizeof(ssidList)/sizeof(ssidList[0]); i++) {
        WiFi.begin(ssidList[i], passwordList[i]);
        unsigned long start = millis();
        while (millis() - start < 10000 && WiFi.status() != WL_CONNECTED) {
            delay(500);
        }
        if (WiFi.status() == WL_CONNECTED) {
            wifiSSID = ssidList[i];
            Serial.printf("Connected to %s, IP: %s\n", wifiSSID, WiFi.localIP().toString().c_str());
            sensorData.errorFlags[0] &= ~ERR_WIFI;
            return true;
        }
    }
    sensorData.errorFlags[0] |= ERR_WIFI;
    Serial.println("WiFi connection failed");
    return false;
}

void setupNTP() {
    myTZ.setPosix(TZ_STRING);
    events();
    setInterval(NTP_UPDATE_INTERVAL / 1000);
    setServer(ntpServers[0]);
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=== TEST SERIAL OUTPUT ===");
    Serial.flush();
    delay(500);
    Serial.println("Baud: 115200 - if you see this, filters are off");
    Serial.flush();
    Serial.println("Starting setup...");

    Serial.println("Initializing globals...");
    initGlobals();
    Serial.println("Globals initialized");

    // Initialize modules
    Serial.println("Initializing I2C...");
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);
    Serial.println("I2C setup done");

    Serial.println("Initializing sensors...");
    if (!initSensors()) {
        Serial.println("Sensor init failed");
    } else {
        Serial.println("Sensors initialized");
    }

    Serial.println("Initializing display...");
    if (!initDisplay()) {
        Serial.println("Display init failed");
    } else {
        Serial.println("Display initialized");
    }

    Serial.println("Initializing SD card...");
    if (!initSD()) {
        Serial.println("SD init failed");
    } else {
        Serial.println("SD card initialized");
    }

    Serial.println("Setting up WiFi...");
    bool wifiConnected = setupWiFi();
    Serial.println("WiFi setup complete");

    Serial.println("Setting up HTTP server...");
    if (!setupServer()) {
        Serial.println("HTTP server init failed");
    } else {
        Serial.println("HTTP server initialized");
    }

    if (wifiConnected) {
        Serial.println("Setting up NTP...");
        setupNTP();
        for (int i = 0; i < NTP_SERVER_COUNT; i++) {
            setServer(ntpServers[i]);
            updateNTP();
            timeSynced = true;  // Assume success
            break;
            delay(1000);
        }
        if (!timeSynced) {
            Serial.println("NTP sync failed");
        }
        Serial.println("NTP setup complete");
    }

    Serial.println("Loading settings...");
    loadSettings();
    Serial.println("Settings loaded");

    // Enable watchdog
    esp_task_wdt_init(10, true); // 10s timeout, panic on timeout
    esp_task_wdt_add(NULL);

    Serial.println("System initialized successfully");
}

void loop() {
    uint32_t now = millis();

    static uint32_t lastStatusLog = 0;
    if (now - lastStatusLog > 60000) {
      Serial.printf("Status - millis: %lu, heap: %d\n", now, ESP.getFreeHeap());
      Serial.flush();
      lastStatusLog = now;
    }

    // Ensure lv_timer_handler every 5ms
    static uint32_t lastLvgl = 0;
    if (now - lastLvgl >= 5) {
        lv_timer_handler();
        Serial.flush();
        lastLvgl = now;
    }

    // Touch buffer update every 50ms
    static uint32_t lastTouch = 0;
    if (now - lastTouch >= 50) {
        Touch_Read_Data();
        if (millis() - last_touch_time >= 100) {
            Touch_Get_XY(&last_touch_x, &last_touch_y, &touch_strength, &point_num, max_point_num);
            if (point_num > 0) {
                last_touch_time = millis();
            }
        }
        lastTouch = now;
    }

    // Periodic sensor reading
    if (now - lastSensorRead >= SENSOR_READ_INTERVAL) {
        readSensors();
        // saveHistory(); // removed old function
        lastSensorRead = now;
    }

    // Periodic history save
    if (now - lastHistorySave >= HISTORY_INTERVAL) {
        saveHistorySens();
        lastHistorySave = millis();
    }

    // Periodic sensor reset if error
    if ((sensorData.errorFlags[0] & ERR_SENSOR) && now - lastSensorReset >= 60000) {
        Serial.println("[Main] Resetting sensors due to error");
        resetSensors();
        lastSensorReset = now;
    }

    // WiFi reconnect logic
    static unsigned long lastWiFiCheck = 0;
    if (now - lastWiFiCheck > WIFI_CHECK_INTERVAL) {
        lastWiFiCheck = now;
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected, attempting reconnect");
            for (int attempt = 0; attempt < WIFI_RETRY_COUNT; attempt++) {
                if (setupWiFi()) {
                    sensorData.errorFlags[0] &= ~ERR_WIFI;
                    break;
                }
                delay(WIFI_FIXED_DELAY);
            }
            if (WiFi.status() != WL_CONNECTED) {
                sensorData.errorFlags[0] |= ERR_WIFI;
                delay(30000);
            }
        }
    }

    // NTP update logic
    static unsigned long lastNTPUpdate = 0;
    if (WiFi.status() == WL_CONNECTED && now - lastNTPUpdate > NTP_UPDATE_INTERVAL) {
        lastNTPUpdate = now;
        for (int i = 0; i < NTP_SERVER_COUNT; i++) {
            setServer(ntpServers[i]);
            updateNTP();
            break;  // Assume success
            delay(1000);
        }
    }

    // Periodic HTTP send to CEW
    if (now - lastHttpSend >= HTTP_SEND_INTERVAL) {
        JsonDocument doc;
        doc["localTemp"] = sensorData.localTemp;
        doc["localHumidity"] = sensorData.localHumidity;
        doc["localCO2"] = sensorData.localCO2;
        doc["extTemp"] = sensorData.extTemp;
        // Add other fields

        String jsonString;
        serializeJson(doc, jsonString);
        sendWithRetry("http://192.168.2.192/api/sensor-data", jsonString);
        lastHttpSend = now;
    }

    // Check for SEW timeout
    if (millis() - lastSEWReceive > 720000) {
        sensorData.errorFlags[0] |= ERR_HTTP;
    }

    // Check for CE STATUS_UPDATE timeout
    if (millis() - lastStatusUpdate > 300000) {
        sensorData.errorFlags[1] |= ERR_HTTP;
    }

    // Heartbeat ping
    if (now - lastHeartbeat >= HTTP_HEARTBEAT) {
        sendHeartbeat();
        lastHeartbeat = now;
    }

    // Weather update
    if (now - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL) {
        fetchWeather();
        lastWeatherUpdate = now;
    }

    // Archive data at 00:05
    if (now - lastArchive >= 60000) {  // Check every minute
        // archiveData(); // removed old function
        lastArchive = now;
    }

    // Handle HTTP clients
    handleClient();

    // Reset watchdog
    esp_task_wdt_reset();

    delay(1); // Small delay to prevent busy loop
}
