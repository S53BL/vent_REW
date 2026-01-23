// web.cpp
#include "web.h"
#include <ArduinoJson.h>
#include "html.h"
#include <EEPROM.h>
#include "485.h"

AsyncWebServer server(80);

bool sendingHTTP = false;
int httpStatus = 0;
unsigned long httpStatusTime = 0;
bool webServerRunning = false;
extern bool waitingForConfirmAction;
unsigned long lastParamUpdateSent = 0;
String lastRequestType = "";
bool confirmReceived = false;

extern SensorData sensorData;
Settings newSettings;

void handleRoot(AsyncWebServerRequest *request) {
  Serial.println("[Web] Prejet zahtevek: GET /");
  request->send(200, "text/html", index_html);
}

void handleGetSettings(AsyncWebServerRequest *request) {
  Serial.println("[Web] Prejet zahtevek: GET /settings");
  request->send(200, "text/html", index_html);
}

void handleDataRequest(AsyncWebServerRequest *request) {
  Serial.println("[Web] Prejet zahtevek: GET /data");
  Settings settings;
  EEPROM.get(0, settings);
  // Dodana validacija za vrnitev default, če izven razpona (npr. za co2ThresholdHighDS)
  if (settings.co2ThresholdHighDS < 400 || settings.co2ThresholdHighDS > 2000) {
    settings.co2ThresholdHighDS = 1200;  // Vrni default, ne shrani takoj
    Serial.println("[Web] Popravek v /data: co2ThresholdHighDS vrnjen na 1200");
  }
  // Dodaj podobne preverbe za druge parametre po potrebi

  String json = "{" +
                String("\"HUM_THRESHOLD\":\"") + String((int)settings.humThreshold) + "\"," +
                String("\"FAN_DURATION\":\"") + String(settings.fanDuration) + "\"," +
                String("\"FAN_OFF_DURATION\":\"") + String(settings.fanOffDuration) + "\"," +
                String("\"FAN_OFF_DURATION_KOP\":\"") + String(settings.fanOffDurationKop) + "\"," +
                String("\"TEMP_LOW_THRESHOLD\":\"") + String((int)settings.tempLowThreshold) + "\"," +
                String("\"TEMP_MIN_THRESHOLD\":\"") + String((int)settings.tempMinThreshold) + "\"," +
                String("\"DND_ALLOW_AUTOMATIC\":") + String(settings.dndAllowAutomatic ? "true" : "false") + "," +
                String("\"DND_ALLOW_SEMIAUTOMATIC\":") + String(settings.dndAllowSemiautomatic ? "true" : "false") + "," +
                String("\"DND_ALLOW_MANUAL\":") + String(settings.dndAllowManual ? "true" : "false") + "," +
                String("\"CYCLE_DURATION_DS\":\"") + String(settings.cycleDurationDS) + "\"," +
                String("\"CYCLE_ACTIVE_PERCENT_DS\":\"") + String((int)settings.cycleActivePercentDS) + "\"," +
                String("\"HUM_THRESHOLD_DS\":\"") + String((int)settings.humThresholdDS) + "\"," +
                String("\"HUM_THRESHOLD_HIGH_DS\":\"") + String((int)settings.humThresholdHighDS) + "\"," +
                String("\"HUM_EXTREME_HIGH_DS\":\"") + String((int)settings.humExtremeHighDS) + "\"," +
                String("\"CO2_THRESHOLD_LOW_DS\":\"") + String(settings.co2ThresholdLowDS) + "\"," +
                String("\"CO2_THRESHOLD_HIGH_DS\":\"") + String(settings.co2ThresholdHighDS) + "\"," +
                String("\"INCREMENT_PERCENT_LOW_DS\":\"") + String((int)settings.incrementPercentLowDS) + "\"," +
                String("\"INCREMENT_PERCENT_HIGH_DS\":\"") + String((int)settings.incrementPercentHighDS) + "\"," +
                String("\"INCREMENT_PERCENT_TEMP_DS\":\"") + String((int)settings.incrementPercentTempDS) + "\"," +
                String("\"TEMP_IDEAL_DS\":\"") + String((int)settings.tempIdealDS) + "\"," +
                String("\"TEMP_EXTREME_HIGH_DS\":\"") + String((int)settings.tempExtremeHighDS) + "\"," +
                String("\"TEMP_EXTREME_LOW_DS\":\"") + String((int)settings.tempExtremeLowDS) + "\"}";
  request->send(200, "application/json", json);
}

void handlePostSettings(AsyncWebServerRequest *request) {
  Serial.println("[Web] Prejet zahtevek: POST /settings/update");
  String errorMessage = "";

  if (!request->hasParam("humThreshold", true) || !request->hasParam("fanDuration", true) ||
      !request->hasParam("fanOffDuration", true) || !request->hasParam("fanOffDurationKop", true) ||
      !request->hasParam("tempLowThreshold", true) || !request->hasParam("tempMinThreshold", true) ||
      !request->hasParam("cycleDurationDS", true) ||
      !request->hasParam("cycleActivePercentDS", true) || !request->hasParam("humThresholdDS", true) ||
      !request->hasParam("humThresholdHighDS", true) || !request->hasParam("humExtremeHighDS", true) ||
      !request->hasParam("co2ThresholdLowDS", true) || !request->hasParam("co2ThresholdHighDS", true) ||
      !request->hasParam("incrementPercentLowDS", true) || !request->hasParam("incrementPercentHighDS", true) ||
      !request->hasParam("incrementPercentTempDS", true) || !request->hasParam("tempIdealDS", true) ||
      !request->hasParam("tempExtremeHighDS", true) || !request->hasParam("tempExtremeLowDS", true)) {
    errorMessage = "Manjkajoči parametri";
    Serial.println("[Web] Validacijska napaka: " + errorMessage);
    request->send(400, "text/plain", errorMessage);
    return;
  }

  newSettings.humThreshold = request->getParam("humThreshold", true)->value().toFloat();
  newSettings.fanDuration = request->getParam("fanDuration", true)->value().toInt();
  newSettings.fanOffDuration = request->getParam("fanOffDuration", true)->value().toInt();
  newSettings.fanOffDurationKop = request->getParam("fanOffDurationKop", true)->value().toInt();
  newSettings.tempLowThreshold = request->getParam("tempLowThreshold", true)->value().toFloat();
  newSettings.tempMinThreshold = request->getParam("tempMinThreshold", true)->value().toFloat();
  newSettings.dndAllowAutomatic = request->hasParam("dndAllowAutomatic", true);  // Fix: true če hasParam, sicer false
  newSettings.dndAllowSemiautomatic = request->hasParam("dndAllowSemiautomatic", true);
  newSettings.dndAllowManual = request->hasParam("dndAllowManual", true);
  newSettings.cycleDurationDS = request->getParam("cycleDurationDS", true)->value().toInt();
  newSettings.cycleActivePercentDS = request->getParam("cycleActivePercentDS", true)->value().toFloat();
  newSettings.humThresholdDS = request->getParam("humThresholdDS", true)->value().toFloat();
  newSettings.humThresholdHighDS = request->getParam("humThresholdHighDS", true)->value().toFloat();
  newSettings.humExtremeHighDS = request->getParam("humExtremeHighDS", true)->value().toFloat();
  newSettings.co2ThresholdLowDS = request->getParam("co2ThresholdLowDS", true)->value().toInt();
  newSettings.co2ThresholdHighDS = request->getParam("co2ThresholdHighDS", true)->value().toInt();
  newSettings.incrementPercentLowDS = request->getParam("incrementPercentLowDS", true)->value().toFloat();
  newSettings.incrementPercentHighDS = request->getParam("incrementPercentHighDS", true)->value().toFloat();
  newSettings.incrementPercentTempDS = request->getParam("incrementPercentTempDS", true)->value().toFloat();
  newSettings.tempIdealDS = request->getParam("tempIdealDS", true)->value().toFloat();
  newSettings.tempExtremeHighDS = request->getParam("tempExtremeHighDS", true)->value().toFloat();
  newSettings.tempExtremeLowDS = request->getParam("tempExtremeLowDS", true)->value().toFloat();

  // Dodane specifične error message za relacije
  if (newSettings.humThreshold >= newSettings.humExtremeHighDS) {
    errorMessage = "Napaka: humThreshold mora biti < humExtremeHighDS";
    Serial.println("[Web] Validacijska napaka: " + errorMessage);
    request->send(400, "text/plain", errorMessage);
    return;
  }
  if (newSettings.humThresholdDS >= newSettings.humThresholdHighDS) {
    errorMessage = "Napaka: humThresholdDS mora biti < humThresholdHighDS";
    Serial.println("[Web] Validacijska napaka: " + errorMessage);
    request->send(400, "text/plain", errorMessage);
    return;
  }
  if (newSettings.humThresholdHighDS >= newSettings.humExtremeHighDS) {
    errorMessage = "Napaka: humThresholdHighDS mora biti < humExtremeHighDS";
    Serial.println("[Web] Validacijska napaka: " + errorMessage);
    request->send(400, "text/plain", errorMessage);
    return;
  }
  if (newSettings.co2ThresholdLowDS >= newSettings.co2ThresholdHighDS) {
    errorMessage = "Napaka: co2ThresholdLowDS mora biti < co2ThresholdHighDS";
    Serial.println("[Web] Validacijska napaka: " + errorMessage);
    request->send(400, "text/plain", errorMessage);
    return;
  }
  if (newSettings.tempMinThreshold >= newSettings.tempLowThreshold) {
    errorMessage = "Napaka: tempMinThreshold mora biti < tempLowThreshold";
    Serial.println("[Web] Validacijska napaka: " + errorMessage);
    request->send(400, "text/plain", errorMessage);
    return;
  }
  if (newSettings.tempLowThreshold >= newSettings.tempIdealDS) {
    errorMessage = "Napaka: tempLowThreshold mora biti < tempIdealDS";
    Serial.println("[Web] Validacijska napaka: " + errorMessage);
    request->send(400, "text/plain", errorMessage);
    return;
  }
  if (newSettings.tempIdealDS >= newSettings.tempExtremeHighDS) {
    errorMessage = "Napaka: tempIdealDS mora biti < tempExtremeHighDS";
    Serial.println("[Web] Validacijska napaka: " + errorMessage);
    request->send(400, "text/plain", errorMessage);
    return;
  }

  uint8_t data[85];
  data[0] = PARAM_UPDATE;
  memcpy(&data[1], &newSettings, sizeof(Settings));
  sendViaRS485(data, sizeof(data));
  Serial.println("[Web] PARAM_UPDATE poslan: humThreshold=" + String(newSettings.humThreshold, 1) +
                 ", fanDuration=" + String(newSettings.fanDuration) +
                 ", fanOffDuration=" + String(newSettings.fanOffDuration) +
                 ", fanOffDurationKop=" + String(newSettings.fanOffDurationKop) +
                 ", tempLowThreshold=" + String(newSettings.tempLowThreshold, 1) +
                 ", tempMinThreshold=" + String(newSettings.tempMinThreshold, 1) +
                 ", dndAllowAutomatic=" + String(newSettings.dndAllowAutomatic) +
                 ", dndAllowSemiautomatic=" + String(newSettings.dndAllowSemiautomatic) +
                 ", dndAllowManual=" + String(newSettings.dndAllowManual) +
                 ", cycleDurationDS=" + String(newSettings.cycleDurationDS) +
                 ", cycleActivePercentDS=" + String(newSettings.cycleActivePercentDS, 1) +
                 ", humThresholdDS=" + String(newSettings.humThresholdDS, 1) +
                 ", humThresholdHighDS=" + String(newSettings.humThresholdHighDS, 1) +
                 ", humExtremeHighDS=" + String(newSettings.humExtremeHighDS, 1) +
                 ", co2ThresholdLowDS=" + String(newSettings.co2ThresholdLowDS) +
                 ", co2ThresholdHighDS=" + String(newSettings.co2ThresholdHighDS) +
                 ", incrementPercentLowDS=" + String(newSettings.incrementPercentLowDS, 1) +
                 ", incrementPercentHighDS=" + String(newSettings.incrementPercentHighDS, 1) +
                 ", incrementPercentTempDS=" + String(newSettings.incrementPercentTempDS, 1) +
                 ", tempIdealDS=" + String(newSettings.tempIdealDS, 1) +
                 ", tempExtremeHighDS=" + String(newSettings.tempExtremeHighDS, 1) +
                 ", tempExtremeLowDS=" + String(newSettings.tempExtremeLowDS, 1));

  waitingForConfirmAction = true;
  lastParamUpdateSent = millis();
  lastRequestType = "PARAM_UPDATE";
  confirmReceived = false;
  request->send(200, "text/plain", "Zahtevek poslan, čakanje na potrditev CE");
}

void handleResetSettings(AsyncWebServerRequest *request) {
  Serial.println("[Web] Prejet zahtevek: POST /settings/reset");
  newSettings = defaultSettings;

  uint8_t data[85];
  data[0] = PARAM_UPDATE;
  memcpy(&data[1], &newSettings, sizeof(Settings));
  sendViaRS485(data, sizeof(data));
  Serial.println("[Web] Privzete nastavitve poslane prek RS485");

  waitingForConfirmAction = true;
  lastParamUpdateSent = millis();
  lastRequestType = "PARAM_UPDATE";
  confirmReceived = false;
  request->send(200, "text/plain", "Zahtevek poslan, čakanje na potrditev CE");
}

void handleHistoryRequest(AsyncWebServerRequest *request) {
  Serial.println("[Web] Prejet zahtevek: POST /history");
  String errorMessage = "";

  if (!request->hasParam("historyDate", true)) {
    errorMessage = "Manjka parameter: historyDate";
    Serial.println("[Web] Validacijska napaka: " + errorMessage);
    request->send(400, "text/plain", errorMessage);
    return;
  }

  String dateStr = request->getParam("historyDate", true)->value();
  if (dateStr.length() != 8 || !dateStr.toInt()) {
    errorMessage = "Neveljaven format datuma: mora biti YYYYMMDD";
    Serial.println("[Web] Validacijska napaka: " + errorMessage);
    request->send(400, "text/plain", errorMessage);
    return;
  }

  uint32_t year = dateStr.substring(0, 4).toInt();
  uint32_t month = dateStr.substring(4, 6).toInt();
  uint32_t day = dateStr.substring(6, 8).toInt();
  if (year != 2025 || month < 1 || month > 12 || day < 1 || day > 31) {
    errorMessage = "Neveljaven datum: mora biti med 20250101 in 20250711";
    Serial.println("[Web] Validacijska napaka: " + errorMessage);
    request->send(400, "text/plain", errorMessage);
    return;
  }
  uint32_t yyyymmdd = year * 10000 + month * 100 + day;

  Serial.println("[Web] Pošiljam HISTORY_REQUEST: Datum=" + dateStr);
  uint8_t data[6];
  data[0] = HISTORY_REQUEST;
  data[1] = (yyyymmdd >> 24) & 0xFF;
  data[2] = (yyyymmdd >> 16) & 0xFF;
  data[3] = (yyyymmdd >> 8) & 0xFF;
  data[4] = yyyymmdd & 0xFF;
  data[5] = 0x00;
  sendViaRS485(data, sizeof(data));

  waitingForConfirmAction = true;
  lastParamUpdateSent = millis();
  lastRequestType = "HISTORY_REQUEST";
  confirmReceived = false;
  request->send(200, "text/plain", "Zahtevek poslan, čakanje na potrditev CE");
}

void handleHistoryData(AsyncWebServerRequest *request) {
  Serial.println("[Web] Prejet zahtevek: GET /history/data");
  if (historyBufferSize > 0) {
    Serial.println("[Web] Pošiljam HISTORY_RESPONSE: Velikost=" + String(historyBufferSize) + " bajtov");
    request->send(200, "application/zip", historyBuffer, historyBufferSize);
    historyBufferSize = 0;
  } else {
    Serial.println("[Web] Napaka: Brez podatkov za prenos zgodovine");
    request->send(404, "text/plain", "Ni podatkov za izbrani datum");
  }
}

void handleSettingsStatus(AsyncWebServerRequest *request) {
  Serial.println("[Web] Prejet zahtevek: GET /settings/status");
  DynamicJsonDocument doc(200);
  if (!waitingForConfirmAction) {
    doc["waiting"] = false;
    if (confirmReceived) {
      doc["success"] = true;
      doc["message"] = lastRequestType == "PARAM_UPDATE" ? "Nastavitve shranjene!" : "Prenos uspel!";
    } else {
      doc["success"] = false;
      doc["message"] = lastRequestType == "PARAM_UPDATE" ? "Napaka: Timeout brez potrditve" : "Napaka: Timeout brez potrditve";
    }
  } else {
    doc["waiting"] = true;
    doc["success"] = false;
    doc["message"] = "Zahtevek poslan, čakanje na potrditev CE";
  }
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

void handleGetData(AsyncWebServerRequest *request) {
  Serial.println("[Web] Prejet zahtevek: GET /data (stari endpoint)");
  String json = "{\"temp\":\"" + (sensorData.extTemp != 0 ? String(sensorData.extTemp, 1) : "N/A") + "\"," +
                "\"humidity\":\"" + (sensorData.extHumidity != 0 ? String(sensorData.extHumidity, 1) : "N/A") + "\"," +
                "\"pressure\":\"" + (sensorData.extPressure != 0 ? String(sensorData.extPressure, 1) : "N/A") + "\"," +
                "\"voc\":\"" + (sensorData.extVOC != 0 ? String(sensorData.extVOC, 1) : "N/A") + "\"}";
  request->send(200, "application/json", json);
}

void handlePostData(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  sendingHTTP = true;
  Serial.println("[Web] POST /data prejet: len=" + String(len));

  String json = "";
  for (size_t i = 0; i < len; i++) {
    json += (char)data[i];
  }
  Serial.println("[Web] POST vsebina: " + json);

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, json);
  if (!error) {
    sensorData.prevExtTemp = sensorData.extTemp;
    sensorData.prevExtHumidity = sensorData.extHumidity;
    sensorData.prevExtPressure = sensorData.extPressure;
    sensorData.prevExtVOC = sensorData.extVOC;
    sensorData.extTemp = doc["temp"] | 0.0;
    sensorData.extHumidity = doc["humidity"] | 0.0;
    sensorData.extPressure = doc["pressure"] | 0.0;
    sensorData.extVOC = doc["voc"] | 0.0;
    Serial.println("[Web] JSON: T=" + String(sensorData.extTemp) + " H=" + String(sensorData.extHumidity) +
                   " P=" + String(sensorData.extPressure) + " VOC=" + String(sensorData.extVOC));
    httpStatus = 1;
    request->send(200, "text/plain", "OK");
  } else {
    Serial.println("[Web] Napaka JSON: " + String(error.c_str()));
    httpStatus = -1;
    request->send(400, "text/plain", "Invalid JSON");
  }
  httpStatusTime = millis();
  sendingHTTP = false;
}

void setupWebServer() {
  Serial.println("[Web] Inicializiram spletni strežnik...");
  Serial.println("[Web] Začenjam server.begin()...");
  server.on("/settings/status", HTTP_GET, handleSettingsStatus);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/settings", HTTP_GET, handleGetSettings);
  server.on("/data", HTTP_GET, handleDataRequest);
  server.on("/settings/update", HTTP_POST, handlePostSettings);
  server.on("/settings/reset", HTTP_POST, handleResetSettings);
  server.on("/history", HTTP_POST, handleHistoryRequest);
  server.on("/history/data", HTTP_GET, handleHistoryData);
  server.on("/data", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      Serial.println("[Web] Prejet zahtevek: POST /data");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {},
    handlePostData);
  server.begin();
  Serial.println("[Web] server.begin() končan");
  webServerRunning = true;
  Serial.println("[Web] Spletni strežnik zagnan na portu 80");
}
// web.cpp