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
        sendToCEW();
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

bool sendWithRetry(String url, String json) {
  for (int i = 0; i < 3; i++) {
    HTTPClient http;
    http.begin(url);
    int httpResponseCode;
    if (json != "") {
      http.addHeader("Content-Type", "application/json");
      httpResponseCode = http.POST(json);
    } else {
      httpResponseCode = http.GET();
    }
    http.end();
    if (httpResponseCode == 200) {
      if (json != "") Serial.println("HTTP success to CEW");
      else Serial.println("Heartbeat success");
      return true;
    } else {
      if (i == 0) {
        if (json != "") Serial.printf("HTTP error to CEW: %d\n", httpResponseCode);
        else Serial.printf("Heartbeat error: %d\n", httpResponseCode);
      }
      if (i < 2) {
        delay((1UL << i) * 1000); // 1s, 2s, 4s
      }
    }
  }
  return false;
}

void sendToCEW() {
    if (WiFi.status() != WL_CONNECTED) {
        sensorData.errorFlags[0] |= ERR_HTTP;
        return;
    }
    HTTPClient http;
    http.begin("http://192.168.2.192/api/sensor-data");
    http.addHeader("Content-Type", "application/json");
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
    int response = http.POST(json);
    if (response == 200) {
        Serial.println("[HTTP] SENSOR_DATA sent to CEW");
        sensorData.errorFlags[0] &= ~ERR_HTTP;
    } else {
        sensorData.errorFlags[0] |= ERR_HTTP;
        Serial.printf("[HTTP] Send to CEW failed: %d\n", response);
    }
    http.end();
    extern void updateUI();
    updateUI();
}

void sendHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected - skipping heartbeat");
    return;
  }
  sendWithRetry("http://192.168.2.192/api/ping", "");
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
