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
#include "logging.h"
#include <Touch_CST328.h>

TwoWire WireTouch = TwoWire(TOUCH_I2C_BUS);

// Touch buffer
uint16_t touch_strength = 0;
uint8_t point_num = 0;
uint8_t max_point_num = 1;

bool setupWiFi() {
    Serial.println("WiFi:Configuring static IP...");
    WiFi.config(localIP, gateway, subnet, dns);
    Serial.println("WiFi:Static IP configured");

    int numNetworks = sizeof(ssidList)/sizeof(ssidList[0]);
    Serial.printf("WiFi:Trying %d networks...\n", numNetworks);

    for (int i = 0; i < numNetworks; i++) {
        Serial.printf("WiFi:Trying network %d: %s\n", i+1, ssidList[i]);
        WiFi.begin(ssidList[i], passwordList[i]);

        unsigned long start = millis();
        int attempts = 0;
        while (millis() - start < 15000) {  // Increased timeout to 15s
            wl_status_t status = WiFi.status();
            Serial.printf("WiFi:Status=%d (attempt %d)\n", status, ++attempts);
            if (status == WL_CONNECTED) {
                wifiSSID = ssidList[i];
                Serial.printf("WiFi:Connected! IP=%s\n", WiFi.localIP().toString().c_str());
                logEvent("WiFi:Connected to " + wifiSSID + " IP=" + WiFi.localIP().toString());
                sensorData.errorFlags[0] &= ~ERR_WIFI;
                return true;
            }
            delay(1000);  // Log every second
        }
        Serial.printf("WiFi:Network %s timeout\n", ssidList[i]);
        WiFi.disconnect();
        delay(1000);
    }
    Serial.println("WiFi:All networks failed");
    sensorData.errorFlags[0] |= ERR_WIFI;
    logEvent("WiFi:Connection failed - all networks tried");
    return false;
}

void setupNTP() {
    myTZ.setPosix(TZ_STRING);
    events();
    setInterval(NTP_UPDATE_INTERVAL / 1000);
    setServer(ntpServers[0]);
}

void setup() {
    // 1. Serial init (Serial-only, pre-logging)
    Serial.begin(115200);
    Serial.println("\n\n=== TEST SERIAL OUTPUT ===");
    Serial.flush();
    delay(500);
    Serial.println("Baud: 115200 - if you see this, filters are off");
    Serial.flush();

    // 2. Basic structures
    Serial.println("Initializing globals...");
    initGlobals();
    Serial.println("Globals init complete");

    // 3. Early logging initialization
    Serial.println("Initializing logging...");
    initLogging();
    logEvent("Setup:Logging initialized");
    Serial.println("Logging init complete");

    // 4. Initialize modules (original sequence)
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

    // 5. WiFi + NTP setup (after hardware init)
    Serial.println("Setting up WiFi...");
    bool wifiConnected = setupWiFi();
    Serial.println("WiFi setup complete");

    if (wifiConnected) {
        sendHeartbeat();
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

    Serial.println("Setting up HTTP server...");
    if (!setupServer()) {
        Serial.println("HTTP server init failed");
    } else {
        Serial.println("HTTP server initialized");
    }

    logEvent("Setup:Loading settings");
    loadSettings();

    // Enable watchdog
    esp_task_wdt_init(10, true); // 10s timeout, panic on timeout
    esp_task_wdt_add(NULL);

    logEvent("Setup:Complete - system ready");
}

void loop() {
    uint32_t now = millis();

    static unsigned long lastHeartbeat = 0;
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
        lastTouch = now;
    }

    // Periodic sensor reading
    if (now - lastSensorRead >= SENSOR_READ_INTERVAL) {
        readSensors();
        lastSensorRead = now;
    }

    // Periodic history save
    if (now - lastHistorySave >= HISTORY_INTERVAL) {
        saveHistorySens();
        lastHistorySave = millis();
    }

    // Periodic sensor reset if error
    if ((sensorData.errorFlags[0] & ERR_SENSOR) && now - lastSensorReset >= 60000) {
        logEvent("Main:Resett sensors due to error");
        resetSensors();
        lastSensorReset = now;
    }

    // WiFi reconnect logic
    static unsigned long lastWiFiCheck = 0;
    if (now - lastWiFiCheck > WIFI_CHECK_INTERVAL) {
        lastWiFiCheck = now;
        if (WiFi.status() != WL_CONNECTED) {
            connection_ok = false;
            logEvent("WiFi:Disconnected - attempting reconnect");
            for (int attempt = 0; attempt < WIFI_RETRY_COUNT; attempt++) {
                if (setupWiFi()) {
                    sensorData.errorFlags[0] &= ~ERR_WIFI;
                    connection_ok = true;
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
        DynamicJsonDocument doc(512);
        doc["extTemp"] = sensorData.extTemp;
        doc["extHumidity"] = sensorData.extHumidity;
        doc["extPressure"] = sensorData.extPressure;
        doc["dsTemp"] = sensorData.localTemp;
        doc["dsHumidity"] = sensorData.localHumidity;
        doc["dsCO2"] = sensorData.localCO2;
        doc["extLux"] = sensorData.extLux;
        String json;
        serializeJson(doc, json);
        sendToCEW("POST", "/api/sensor-data", json);
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
    if (now - lastHeartbeat >= 300000) {
        sendToCEW("GET", "/api/ping", "");
        lastHeartbeat = now;
    }

    // Weather update
    if (now - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL) {
        fetchWeather();
        lastWeatherUpdate = now;
    }

    // Handle HTTP clients
    handleClient();

    // Reset watchdog
    esp_task_wdt_reset();

    delay(1); // Small delay to prevent busy loop
}
