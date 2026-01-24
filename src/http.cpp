// http.cpp - HTTP communication module implementation
#include "http.h"
#include "globals.h"
#include "sd.h"
#include "logging.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);

// Flag to track first HTTP attempt after boot (longer timeout)
static bool firstHttpAttempt = true;

// Forward declaration for lambda functions
extern void logEvent(String content);

bool setupServer() {
    logEvent("HTTP:Setting up server endpoints");
    // Endpoint for receiving data from external unit
    server.on("/data", HTTP_POST, [](AsyncWebServerRequest *request){
        String body = request->arg("plain");
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            request->send(400, "text/plain", "Invalid JSON");
            return;
        }
        sensorData.extTemp = doc["temp"] | 0.0f;
        sensorData.extHumidity = doc["humidity"] | 0.0f;
        sensorData.extPressure = doc["pressure"] | 0.0f;
        sensorData.extVOC = doc["voc"] | 0.0f;
        sensorData.extLux = doc["lux"] | 0.0f;
        logEvent("HTTP:Recv SEW temp=" + String(sensorData.extTemp, 1) +
                 " hum=" + String(sensorData.extHumidity, 1) +
                 " pressure=" + String(sensorData.extPressure, 1) +
                 " voc=" + String(sensorData.extVOC, 0) +
                 " lux=" + String(sensorData.extLux, 0));
        lastSEWReceive = millis();
        request->send(200, "application/json", "{\"status\":\"OK\"}");
        // sendToCEW() call removed as it's now parameterized
    });

    // Heartbeat endpoint
    server.on("/api/ping", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "pong");
    });

    // STATUS_UPDATE endpoint
    server.on("/api/status-update", HTTP_POST, [](AsyncWebServerRequest *request){
        String body = request->arg("plain");
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            request->send(400, "text/plain", "Invalid JSON");
            logEvent("HTTP:Invalid STATUS_UPDATE JSON");
            return;
        }
        if (doc.containsKey("fanStates")) {
            JsonArray fanStatesArr = doc["fanStates"];
            for (int i = 0; i < 4; i++) sensorData.fanStates[i] = fanStatesArr[i].as<uint8_t>();
        }
        if (doc.containsKey("fans")) {
            for (int i = 0; i < 6; i++) sensorData.fans[i] = doc["fans"][i].as<uint8_t>();
        }
        if (doc.containsKey("inputs")) {
            for (int i = 0; i < 8; i++) sensorData.inputs[i] = doc["inputs"][i].as<uint8_t>();
        }
        sensorData.bathroomTemp = doc["bathroomTemp"] | 0.0f;
        sensorData.bathroomHumidity = doc["bathroomHumidity"] | 0.0f;
        sensorData.bathroomPressure = doc["bathroomPressure"] | 0.0f;
        sensorData.utTemp = doc["utTemp"] | 0.0f;
        sensorData.utHumidity = doc["utHumidity"] | 0.0f;
        if (doc.containsKey("offTimes")) {
            for (int i = 0; i < 6; i++) sensorData.offTimes[i] = doc["offTimes"][i].as<uint32_t>();
        }
        sensorData.currentPower = doc["currentPower"] | 0.0f;
        sensorData.energyConsumption = doc["energyConsumption"] | 0.0f;
        if (doc.containsKey("errorFlags")) {
            for (int i = 0; i < 5; i++) sensorData.errorFlags[i] = doc["errorFlags"][i].as<uint8_t>();
        }
        logEvent("HTTP:STATUS_UPDATE received power=" + String(sensorData.currentPower, 1));
        lastStatusUpdate = millis();
        request->send(200, "application/json", "{\"status\":\"OK\"}");
        extern void updateCards();
        updateCards();
        saveFanHistory();
    });

    // LOGS endpoint for receiving CEW logs
    server.on("/api/logs", HTTP_POST, [](AsyncWebServerRequest *request){
        String body = request->arg("plain");
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            request->send(400, "text/plain", "Invalid JSON");
            logEvent("HTTP:Invalid LOGS JSON");
            return;
        }

        String logsContent = doc["logs"] | "";
        if (logsContent.length() == 0) {
            request->send(400, "text/plain", "No logs content");
            logEvent("HTTP:No logs content in request");
            return;
        }

        logEvent("HTTP:Received " + String(logsContent.length()) + " bytes of CEW logs");

        // Append CEW logs to current date file
        String currentDate = myTZ.dateTime("Ymd");
        String logFileName = "/logs_" + currentDate + ".txt";

        File logFile = SD_MMC.open(logFileName.c_str(), FILE_APPEND);
        if (!logFile) {
            logEvent("HTTP:Failed to open log file for CEW logs: " + logFileName);
            request->send(500, "text/plain", "Failed to open log file");
            return;
        }

        size_t bytesWritten = logFile.print(logsContent);
        logFile.close();

        if (bytesWritten > 0) {
            logEvent("HTTP:Appended " + String(bytesWritten) + " bytes of CEW logs to " + logFileName);
            // Flush REW buffer after receiving CEW logs
            extern void flushBufferToSD();
            flushBufferToSD();
        } else {
            logEvent("HTTP:Failed to write CEW logs to file");
        }

        request->send(200, "application/json", "{\"status\":\"OK\"}");
    });

    logEvent("HTTP:Starting server");
    server.begin();
    webServerRunning = true;
    logEvent("HTTP:Server started");
    return true;
}

void handleClient() {
    // Async server handles clients automatically
}



void sendToCEW(String method, String endpoint, String jsonPayload) {
    logEvent("HTTP:Send " + method + " to " + endpoint + " payload=" + jsonPayload);

    if (!connection_ok || WiFi.status() != WL_CONNECTED) {
        logEvent("HTTP:Not sent - connection not OK or WiFi err");
        sensorData.errorFlags[0] |= ERR_HTTP;
        return;
    }
    HTTPClient http;

    // Use longer timeout for first attempt after boot
    int timeout = firstHttpAttempt ? 10000 : 2000;
    firstHttpAttempt = false;  // Reset flag after first use

    http.setTimeout(timeout);
    http.setConnectTimeout(timeout);
    String url = "http://" + String(CEW_IP) + endpoint;
    if (!http.begin(url)) {
        logEvent("HTTP:Begin failed for " + url);
        connection_ok = false;
        return;
    }
    int httpCode;
    if (method == "POST") {
        http.addHeader("Content-Type", "application/json");
        httpCode = http.POST(jsonPayload);
    } else if (method == "GET") {
        httpCode = http.GET();
    } else {
        logEvent("HTTP:Invalid method " + method);
        http.end();
        return;
    }
    if (httpCode == HTTP_CODE_OK) {
        lastSuccessfulHeartbeat = millis();
        connection_ok = true;
        logEvent("HTTP:Send OK to " + endpoint);
        sensorData.errorFlags[0] &= ~ERR_HTTP;
    } else {
        logEvent("HTTP:Send failed code=" + String(httpCode) + " to " + endpoint);
        delay(1000);
        yield();
        // 1x retry
        if (method == "POST") {
            httpCode = http.POST(jsonPayload);
        } else {
            httpCode = http.GET();
        }
        if (httpCode == HTTP_CODE_OK) {
            lastSuccessfulHeartbeat = millis();
            connection_ok = true;
            logEvent("HTTP:Send OK (retry) to " + endpoint);
        } else {
            logEvent("HTTP:Not sent - CEW offline");
            connection_ok = false;
            sensorData.errorFlags[0] |= ERR_HTTP;
        }
    }
    http.end();
    // Za GET ignoriraj body, samo status code
}

bool sendHeartbeat() {
    HTTPClient http;

    // Use longer timeout for first attempt after boot
    int timeout = firstHttpAttempt ? 10000 : 2000;
    firstHttpAttempt = false;  // Reset flag after first use

    http.setTimeout(timeout);
    http.setConnectTimeout(timeout);

    String url = "http://" + String(CEW_IP) + "/api/ping";
    if (!http.begin(url)) {
        logEvent("HTTP:Heartbeat begin failed");
        return false;
    }
    int httpCode = http.GET();
    http.end();
    if (httpCode == HTTP_CODE_OK) {
        lastSuccessfulHeartbeat = millis();
        sensorData.errorFlags[0] &= ~ERR_HTTP;
        connection_ok = true;
        logEvent("HTTP:Heartbeat success");
        return true;
    } else {
        logEvent("HTTP:Heartbeat failed code=" + String(httpCode) + " (timeout=" + String(timeout) + "ms)");
        connection_ok = false;
        return false;
    }
}

void fetchWeather() {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    http.begin("https://api.open-meteo.com/v1/forecast?latitude=46.0569&longitude=14.5058&current=weather_code");
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
        String payload = http.getString();
        JsonDocument doc;
        deserializeJson(doc, payload);
        int weatherCode = doc["current"]["weather_code"];

        // Map weather code to icon
        if (weatherCode == 0) sensorData.weatherIcon = "sun.png";
        else if (weatherCode <= 3) sensorData.weatherIcon = "partly-cloudy.png";
        // Add more mappings
        else sensorData.weatherIcon = "cloud.png";
    } else {
        sensorData.errorFlags[0] |= ERR_HTTP;
    }
    http.end();
}



