// http.cpp - HTTP communication module implementation
#include "http.h"
#include "globals.h"
#include "sd.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);

bool setupServer() {
    Serial.println("    Setting up server endpoints...");
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
        Serial.printf("[HTTP] Received from SEW: temp=%.1f, hum=%.1f, pressure=%.1f, voc=%.1f, lux=%.1f\n",
                      sensorData.extTemp, sensorData.extHumidity, sensorData.extPressure, sensorData.extVOC, sensorData.extLux);
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
            Serial.println("[HTTP] Invalid STATUS_UPDATE JSON");
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
        Serial.println("[HTTP] STATUS_UPDATE received: fanStates=[...], power=%.1f, errors=[...]");
        lastStatusUpdate = millis();
        request->send(200, "application/json", "{\"status\":\"OK\"}");
        extern void updateCards();
        updateCards();
        saveFanHistory();
    });

    Serial.println("    Starting server...");
    server.begin();
    webServerRunning = true;
    Serial.println("    Server started");
    return true;
}

void handleClient() {
    // Async server handles clients automatically
}



void sendToCEW(String method, String endpoint, String jsonPayload) {
    Serial.printf("[JSON] %s: %s\n", endpoint.c_str(), jsonPayload.c_str());

    if (!connection_ok || WiFi.status() != WL_CONNECTED) {
        Serial.println("[HTTP] - not sent: connection not ok or WiFi err");
        sensorData.errorFlags[0] |= ERR_HTTP;
        return;
    }
    HTTPClient http;
    http.setTimeout(2000);
    http.setConnectTimeout(2000);  // Set connection timeout to 2000ms
    String url = "http://" + String(CEW_IP) + endpoint;
    if (!http.begin(url)) {
        Serial.println("[HTTP] begin failed");
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
        Serial.println("[HTTP] invalid method");
        http.end();
        return;
    }
    if (httpCode == HTTP_CODE_OK) {
        lastSuccessfulHeartbeat = millis();
        connection_ok = true;
        Serial.println("[HTTP] sent OK");
        sensorData.errorFlags[0] &= ~ERR_HTTP;
    } else {
        Serial.printf("[HTTP] Send failed: %d to %s\n", httpCode, endpoint.c_str());
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
            Serial.println("[HTTP] sent OK (retry)");
        } else {
            Serial.println("[HTTP] not sent: CEW offline");
            connection_ok = false;
            sensorData.errorFlags[0] |= ERR_HTTP;
        }
    }
    http.end();
    // Za GET ignoriraj body, samo status code
}

bool sendHeartbeat() {
    HTTPClient http;
    http.setTimeout(2000);
    http.setConnectTimeout(2000);  // Set connection timeout to 2000ms
    String url = "http://" + String(CEW_IP) + "/api/ping";
    if (!http.begin(url)) {
        Serial.println("[HTTP] Heartbeat begin failed");
        return false;
    }
    int httpCode = http.GET();
    http.end();
    if (httpCode == HTTP_CODE_OK) {
        lastSuccessfulHeartbeat = millis();
        sensorData.errorFlags[0] &= ~ERR_HTTP;
        connection_ok = true;
        Serial.println("[HTTP] Heartbeat success");
        return true;
    } else {
        Serial.printf("[HTTP] Heartbeat failed: %d\n", httpCode);
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



