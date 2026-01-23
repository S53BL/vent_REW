// main.cpp
#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ezTime.h>
#include <Adafruit_SHT4x.h>
#include <SensirionI2cScd4x.h>
#include "config.h"
#include "display.h"
#include "485.h"
#include "web.h"
#include "html.h"
#include "wifi_config.h"


const char* ntpServers[] = {"pool.ntp.org", "time.nist.gov", "europe.pool.ntp.org", "ntp1.inrim.it", "ntp2.inrim.it", "ntp1.arnes.si"};
const int numNTPServers = 6;

const Settings defaultSettings = {
  60.0f, 180, 1200, 600, 5.0f, -10.0f, false, true, true,
  3600, 90.0f, 60.0f, 70.0f, 80.0f, 900, 1200, 15.0f, 50.0f, 20.0f, 24.0f, 30.0f, -10.0f
};

Timezone myTZ;
Adafruit_SHT4x sht41 = Adafruit_SHT4x();
SensirionI2cScd4x scd40;

SensorData sensorData = {
  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
  0.0, 0.0, 0.0, 0.0,
  {0,0,0,0,0}, {0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0}
};
unsigned long lastSensorRead = 0;
unsigned long lastNTPSync = 0;
unsigned long lastValidTime = 0; // Za interni čas
unsigned long lastTimeUpdate = 0;
bool ntpFailed = false;
unsigned long lastWiFiRetry = 0;
unsigned long lastWiFiCheck = 0;
unsigned long lastDiagnostic = 0;
unsigned long lastSensorDataSent = 0;
unsigned long lastGraphUpdate = 0;
static unsigned long lastStateRequest = 0;
extern String lastRequestType; // Dodano za dostop do lastRequestType

void initEEPROM() {
  EEPROM.begin(85);
  uint8_t marker;
  EEPROM.get(84, marker);
  if (marker != 0xAB) {
    Serial.println("[EEPROM] Ni inicializiran, vpisujem privzete vrednosti");
    EEPROM.put(0, defaultSettings);
    EEPROM.put(84, (uint8_t)0xAB);
    EEPROM.commit();
    Serial.println("[EEPROM] Inicializiran");
  } else {
    Serial.println("[EEPROM] Že inicializiran");
    Settings settings;
    EEPROM.get(0, settings);
    bool corrupted = false;

    // Validacija vseh parametrov (dodaj po potrebi za vse 22)
    if (settings.humThreshold < 0 || settings.humThreshold > 100) corrupted = true;
    if (settings.fanDuration < 60 || settings.fanDuration > 6000) corrupted = true;
    if (settings.fanOffDuration < 60 || settings.fanOffDuration > 6000) corrupted = true;
    if (settings.fanOffDurationKop < 60 || settings.fanOffDurationKop > 6000) corrupted = true;
    if (settings.tempLowThreshold < -20 || settings.tempLowThreshold > 40) corrupted = true;
    if (settings.tempMinThreshold < -20 || settings.tempMinThreshold > 40) corrupted = true;
    // Bool ne rabijo validacije, saj so 0/1
    if (settings.cycleDurationDS < 60 || settings.cycleDurationDS > 6000) corrupted = true;
    if (settings.cycleActivePercentDS < 0 || settings.cycleActivePercentDS > 100) corrupted = true;
    if (settings.humThresholdDS < 0 || settings.humThresholdDS > 100) corrupted = true;
    if (settings.humThresholdHighDS < 0 || settings.humThresholdHighDS > 100) corrupted = true;
    if (settings.humExtremeHighDS < 0 || settings.humExtremeHighDS > 100) corrupted = true;
    if (settings.co2ThresholdLowDS < 400 || settings.co2ThresholdLowDS > 2000) corrupted = true;
    if (settings.co2ThresholdHighDS < 400 || settings.co2ThresholdHighDS > 2000) corrupted = true;
    if (settings.incrementPercentLowDS < 0 || settings.incrementPercentLowDS > 100) corrupted = true;
    if (settings.incrementPercentHighDS < 0 || settings.incrementPercentHighDS > 100) corrupted = true;
    if (settings.incrementPercentTempDS < 0 || settings.incrementPercentTempDS > 100) corrupted = true;
    if (settings.tempIdealDS < -20 || settings.tempIdealDS > 40) corrupted = true;
    if (settings.tempExtremeHighDS < -20 || settings.tempExtremeHighDS > 40) corrupted = true;
    if (settings.tempExtremeLowDS < -20 || settings.tempExtremeLowDS > 40) corrupted = true;

    if (corrupted) {
      Serial.println("[EEPROM] Corrupted podatki zaznani, prepisujem z defaultSettings");
      EEPROM.put(0, defaultSettings);
      EEPROM.commit();
    }
  }
}

bool syncNTP() {
  static unsigned long lastNTPSyncAttempt = 0;
  if (millis() - lastNTPSyncAttempt < 300) return false;

  Serial.println("[NTP] Sinhroniziram...");
  unsigned long startTime = millis();
  bool success = false;

  for (int i = 0; i < numNTPServers; i++) {
    Serial.println("[NTP] Poskušam strežnik: " + String(ntpServers[i]));
    setServer(ntpServers[i]);
    for (int attempt = 0; attempt < 3; attempt++) {
      waitForSync(5);
      if (timeStatus() == timeSet) {
        unsigned long epochTime = now();
        if (epochTime > 1609459200) {
          Serial.println("[NTP] Uspeh: " + String(ntpServers[i]) + ", čas: " + String(epochTime) +
                         ", lokalni čas: " + myTZ.dateTime("Y-m-d H:i:s") +
                         ", DST: " + String(myTZ.isDST() ? "CEST" : "CET") +
                         ", zadnja posodobitev: " + String(lastNtpUpdateTime()));
          lastValidTime = epochTime; // Posodobi interni čas
          lastTimeUpdate = millis();
          success = true;
          ntpFailed = false;
          Serial.println("[NTP] Sinhronizacija trajala: " + String(millis() - startTime) + " ms");
          lastNTPSyncAttempt = millis();
          return true;
        } else {
          Serial.println("[NTP] " + String(ntpServers[i]) + ": Neveljaven čas (" + String(epochTime) + ")");
        }
      } else {
        Serial.println("[NTP] " + String(ntpServers[i]) + ": Poskus " + String(attempt + 1) + " neuspešen, status: " + String(timeStatus()));
      }
      lastNTPSyncAttempt = millis();
    }
  }

  Serial.println("[NTP] Sinhronizacija neuspešna");
  ntpFailed = true;
  Serial.println("[NTP] Sinhronizacija trajala: " + String(millis() - startTime) + " ms");
  lastNTPSyncAttempt = millis();
  return false;
}

void connectWiFi() {
  Serial.println("[WiFi] Povezujem...");
  WiFi.disconnect();
  for (int i = 0; i < numNetworks; i++) {
    Serial.println("[WiFi] Poskušam SSID: " + String(ssidList[i]));
    if (!WiFi.config(localIP, gateway, subnet, dns)) {
      Serial.println("[WiFi] Napaka pri konfiguraciji statičnega IP-ja");
    }
    WiFi.begin(ssidList[i], passwordList[i]);
    unsigned long startTime = millis();
    while (millis() - startTime < 10000 && WiFi.status() != WL_CONNECTED) {
      delay(100); // Blokirajoča logika za stabilnost
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[WiFi] Povezan na: " + String(ssidList[i]) + ", IP: " + WiFi.localIP().toString());
      currentSSID = String(ssidList[i]);
      lastWiFiStatus = true;
      return;
    }
    Serial.println("[WiFi] Povezava na " + String(ssidList[i]) + " ni uspela");
  }
  Serial.println("[WiFi] Povezava ni uspela, preklapljam na AP način");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("VentilationAP", "vent12345");
  Serial.println("[WiFi] AP način zagnan: VentilationAP, IP: " + WiFi.softAPIP().toString());
  currentSSID = "VentilationAP";
  lastWiFiStatus = false;
}

void readSensors() {
  if (millis() - lastSensorRead < SENSOR_READ_INTERVAL * 1000) return;
  static unsigned long lastLDRRead = 0;
  static int ldrSampleCount = 0;
  static int ldrSum = 0;

  if (ldrSampleCount < 5) {
    if (millis() - lastLDRRead >= 10) {
      ldrSum += analogRead(PIN_LDR);
      ldrSampleCount++;
      lastLDRRead = millis();
    }
    return;
  }

  Serial.println("[Senzorji] Berem...");
  float ldrValue = ldrSum / 5.0;
  float newLight = 100 - ((ldrValue - 1000) * 100.0 / (4095 - 1000));
  if (ldrValue < 1000) newLight = 100;
  if (abs(newLight - sensorData.prevLocalLight) > 5 || sensorData.prevLocalLight == 0) {
    sensorData.localLight = constrain(newLight, 0, 100);
    Serial.println("[LDR] Svetlost: " + String(sensorData.localLight, 0) + " %");
    sensorData.prevLocalLight = sensorData.localLight;
  }

  sensors_event_t humidity, temp;
  if (sht41.getEvent(&humidity, &temp)) {
    sensorData.localTemp = temp.temperature;
    sensorData.localHumidity = humidity.relative_humidity;
    Serial.println("[SHT41] T=" + String(sensorData.localTemp, 1) + " °C, H=" + String(sensorData.localHumidity, 1) + " %");
  } else {
    sensorData.localTemp = 0.0;
    sensorData.localHumidity = 0.0;
    Serial.println("[SHT41] Napaka pri branju");
  }

  uint16_t co2;
  float scdTemp, scdHumidity;
  uint16_t error = scd40.readMeasurement(co2, scdTemp, scdHumidity);
  if (error == 0 && co2 > 0) {
    sensorData.localCO2 = co2;
    Serial.println("[SCD40] CO2=" + String(sensorData.localCO2, 0) + " ppm");
  } else {
    sensorData.localCO2 = 0.0;
    Serial.println("[SCD40] Napaka pri branju, koda napake: " + String(error));
  }

  lastSensorRead = millis();
  ldrSampleCount = 0;
  ldrSum = 0;
}

void initHardware() {
  Serial.begin(115200);
  Serial.println("[Sistem] Zagon CYD...");
  analogReadResolution(12);

  Serial.println("[I2C] Inicializiram...");
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Serial.println("[SHT41] Preverjam prisotnost...");
  if (sht41.begin()) {
    Serial.println("[SHT41] Uspešno inicializiran");
  } else {
    Serial.println("[SHT41] Napaka: Inicializacija neuspešna");
  }

  Serial.println("[SCD40] Preverjam prisotnost...");
  scd40.begin(Wire, SCD40_ADDRESS);
  scd40.startPeriodicMeasurement();
  Serial.println("[SCD40] Inicializacija začeta");
}

void setup() {
  initHardware();
  initEEPROM();
  initDisplay();
  setupRS485();
  connectWiFi();
  Serial.println("[NTP] Inicializiram...");
  if (WiFi.status() == WL_CONNECTED) {
    myTZ.setLocation("Europe/Ljubljana");
    setInterval(600);
    if (syncNTP()) {
      lastNTPSync = millis();
    }
  } else {
    Serial.println("[NTP] Wi-Fi ni povezan, preskočim");
    ntpFailed = true;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Web] WiFi ni povezan, inicializacija strežnika v AP načinu");
  }
  Serial.println("[Web] Inicializiram strežnik...");
  setupWebServer();
  Serial.println("[Web] Strežnik inicializiran");
  drawTimeAndWiFi();
  drawSensorData();
}

void loop() {
  static unsigned long lastUpdate = 0;
  static unsigned long lastFanTick = 0;
  if (millis() - lastUpdate >= 1000) {
    if (currentDisplayState == DISPLAY_STATE_SQUARES) {
      drawTimeAndWiFi();
      readSensors();
      drawSensorData();
    } else if (currentDisplayState == DISPLAY_STATE_GRAPH && millis() - lastGraphUpdate >= 360000) {
      sendGraphRequest(currentGraphType);
      lastGraphUpdate = millis();
      Serial.println("[Graph] Periodična posodobitev grafa tip " + String((int)currentGraphType));
    }

    // Periodično pošiljanje SENSOR_DATA vsake 6 minut
    if (millis() - lastSensorDataSent >= 360000) {
      sendSensorData();
      lastSensorDataSent = millis();
    }

    // Preverjanje Wi-Fi povezave
    if (millis() - lastWiFiCheck >= 300000) {
      Serial.println("[WiFi] Periodično preverjanje statusa...");
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] Povezava prekinjena, poskušam znova...");
        connectWiFi();
      } else {
        Serial.println("[WiFi] Povezava aktivna: " + String(currentSSID));
      }
      lastWiFiCheck = millis();
    }

    // NTP sinhronizacija
    if (ntpFailed) {
      static unsigned long lastRetry = 0;
      if (lastNTPSync == 0 && millis() - lastRetry >= 5000) {
        if (syncNTP()) {
          lastNTPSync = millis();
        }
        lastRetry = millis();
      } else if (millis() - lastRetry >= 30000) {
        if (syncNTP()) {
          lastNTPSync = millis();
        }
        lastRetry = millis();
      }
    } else if (millis() - lastNTPSync >= 600000) {
      if (syncNTP()) {
        lastNTPSync = millis();
      }
    }

    // Diagnostika
    if (millis() - lastDiagnostic >= 60000) {
      Serial.println("[Diagnostika] Wi-Fi: " + String(WiFi.status() == WL_CONNECTED ? "Povezan (" + currentSSID + ")" : "Nepovezan") +
                     ", Spletni strežnik: " + String(webServerRunning ? "Teče" : "Ne teče"));
      Serial.println("[Diagnostika] TimeStatus: " + String(timeStatus() == timeSet ? "Set" : "NotSet") + ", CurrentTime: " + String(lastValidTime + (millis() - lastTimeUpdate) / 1000));
      lastDiagnostic = millis();
    }

    // RS485 obdelava z izboljšano D/N logiko
    if (dnTriggered && !waitingResponse && millis() - lastStateRequest >= 200) {
      uint8_t stateRequest[] = {STATE_REQUEST}; // STATE_REQUEST (Tip 7)
      sendViaRS485(stateRequest, sizeof(stateRequest));
      Serial.println("[RS485] STATE_REQUEST poslan");
      waitingResponse = true;
      lastRequestSent = millis();
      lastStateRequest = millis();
      retryRequestCount = 0;
    } else if (waitingResponse && millis() - lastRequestSent >= RS485_RESPONSE_TIMEOUT) {
      if (retryRequestCount < 2) {
        uint8_t stateRequest[] = {STATE_REQUEST}; // Ponovi STATE_REQUEST
        sendViaRS485(stateRequest, sizeof(stateRequest));
        Serial.println("[RS485] STATE_REQUEST ponovljen, poskus: " + String(retryRequestCount + 2));
        lastRequestSent = millis();
        lastStateRequest = millis();
        retryRequestCount++;
      } else {
        waitingResponse = false;
        retryRequestCount = 0;
        dnTriggered = false;
        Serial.println("[RS485] Timeout: Brez odgovora po 3 poskusih STATE_REQUEST");
      }
    }

    // Preverjanje timeoutov za ostala sporočila
    if (waitingForConfirmAction && millis() - lastParamUpdateSent > RS485_RESPONSE_TIMEOUT) {
      Serial.println("[RS485] Timeout: Brez potrditve za " + lastRequestType);
      waitingForConfirmAction = false;
      lastRequestType = "";
    }

    if (waitingForManual && millis() - lastManualSent > RS485_RESPONSE_TIMEOUT) {
      Serial.println("[RS485] Timeout: Brez potrditve za MANUAL_CONTROL");
      waitingForManual = false;
    }

    lastUpdate = millis();
  }
  if (millis() - lastFanTick >= 1000) {
    updateFanTimers();  // Sekundno odštevanje
    if (currentDisplayState == DISPLAY_STATE_SQUARES) {
      drawSensorData(); // Osveži ekran vsako sekundo za vidno odštevanje
    }
    lastFanTick = millis();
  }
  receiveRS485();
  handleTouch();
}
// main.cpp