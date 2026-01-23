// src/485.cpp

#include <EEPROM.h>
#include "485.h"
#include "config.h"
#include <CRC.h>
#include <algorithm>
#include "display.h"
#include "web.h"

volatile bool dnTriggered = false;
bool waitingResponse = false;
unsigned long lastRequestSent = 0;
int retryRequestCount = 0;
uint8_t historyBuffer[15000];
size_t historyBufferSize = 0;
extern Settings newSettings;
extern String lastRequestType;
extern unsigned long lastParamUpdateSent;
bool waitingForManual = false;
unsigned long lastManualSent = 0;
bool waitingForConfirmAction = false;

// Tabela pričakovanih dolžin (TIP + PODATKI brez markerjev/CRC)
const size_t expectedLen[] = {
  0,    // 0
  29,   // 1 SENSOR_DATA
  2,    // 2 GRAPH_REQUEST
  969,  // 3 GRAPH_DATA (max)
  3,    // 4 MANUAL_CONTROL
  0,    // 5
  3,    // 6 CONFIRM_ACTION
  1,    // 7 STATE_REQUEST
  73,   // 8 STATUS_UPDATE
  5,    // 9 HISTORY_REQUEST
  0,    // 10 PARAM_CONFIRM
  15013,// 11 HISTORY_RESPONSE
  0,    // 12
  0,    // 13
  3     // 14 PARAM_UPDATE
};

void setupRS485() {
  Serial.println("[RS485] Začenjam inicializacijo");
  Serial2.setRxBufferSize(4096);
  Serial2.begin(RS485_BAUD_RATE, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
  if (!Serial2) {
    Serial.println("[RS485] Napaka (timeout)");
    return;
  }
  pinMode(PIN_RS485_DN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(PIN_RS485_DN), handleDNInterrupt, RISING);
  Serial.println("[RS485] Inicializacija uspešna");
}

void IRAM_ATTR handleDNInterrupt() {
  static unsigned long lastInterruptTime = 0;
  if (millis() - lastInterruptTime >= 200) {
    dnTriggered = true;
    Serial.println("[RS485] D/N signal zaznan (RISING)");
    lastInterruptTime = millis();
  }
}

void receiveRS485() {
  extern SensorData sensorData;
  static uint8_t buffer[4096];
  static size_t bufferIndex = 0;
  static bool foundStartMarker = false;
  const unsigned long READ_TIMEOUT = 500;

  if (Serial2.available()) {
    Serial.println("[RS485] Preverjam prejete podatke...");
    
    unsigned long startTime = millis();
    while (millis() - startTime < READ_TIMEOUT && bufferIndex < sizeof(buffer)) {
      if (!foundStartMarker) {
        int byteRead = Serial2.read();
        if (byteRead == -1) {
          vTaskDelay(1);
          continue;
        }
        if (byteRead == 0x7E) {
          foundStartMarker = true;
          buffer[bufferIndex++] = 0x7E;
        }
        vTaskDelay(1);
        continue;
      }

      int byteRead = Serial2.read();
      if (byteRead == -1) {
        vTaskDelay(1);
        continue;
      }
      buffer[bufferIndex++] = (uint8_t)byteRead;

      if (buffer[bufferIndex - 1] == '\n') {
        // Vedno logiraj raw HEX
        String hexDump = "";
        for (size_t i = 0; i < bufferIndex; i++) {
          hexDump += String(buffer[i], HEX) + " ";
        }
        Serial.println("[RS485] Prejeto sporočilo (surovi bajti HEX): " + hexDump);

        // Preveri začetni marker
        if (buffer[0] != 0x7E) {
          Serial.println("[RS485] ZAVRŽENO: Manjkajoči začetni marker 0x7E");
          bufferIndex = 0;
          foundStartMarker = false;
          break;
        }

        // Preveri končni marker
        if (buffer[bufferIndex - 1] != '\n') {
          Serial.println("[RS485] ZAVRŽENO: Neveljaven končni marker");
          bufferIndex = 0;
          foundStartMarker = false;
          break;
        }

        // Preveri minimalno dolžino
        if (bufferIndex < 5) {
          Serial.println("[RS485] ZAVRŽENO: Prekratko sporočilo, bajtov: " + String(bufferIndex));
          bufferIndex = 0;
          foundStartMarker = false;
          break;
        }

        uint8_t tip = buffer[1];
        if (tip >= sizeof(expectedLen)/sizeof(expectedLen[0]) || tip == 0) {
          Serial.println("[RS485] ZAVRŽENO: Neveljaven tip sporočila: " + String(tip));
          bufferIndex = 0;
          foundStartMarker = false;
          break;
        }

        // Preveri pričakovano dolžino
        size_t expectedTotal = expectedLen[tip] + 1 + 2; // TIP + PODATKI + CRC
        if (bufferIndex != expectedTotal + 1) { // +1 za 0x7E
          Serial.println("[RS485] ZAVRŽENO: Neveljavna dolžina, pričakovano=" + String(expectedTotal + 1) + ", prejeto=" + String(bufferIndex));
          bufferIndex = 0;
          foundStartMarker = false;
          break;
        }

        // Preveri CRC
        uint16_t receivedCRC = (buffer[bufferIndex - 3] << 8) | buffer[bufferIndex - 2];
        uint16_t calculatedCRC = calcCRC16(&buffer[1], bufferIndex - 4, 0x1021);
        if (receivedCRC != calculatedCRC) {
          Serial.println("[RS485] ZAVRŽENO: Neveljaven CRC16, prejet=0x" + String(receivedCRC, HEX) + ", izračunan=0x" + String(calculatedCRC, HEX));
          bufferIndex = 0;
          foundStartMarker = false;
          break;
        }

        Serial.println("[RS485] SPREJETO: CRC16 veljaven");

        uint8_t confirmData[3] = {CONFIRM_ACTION, 0, 0};

        switch (tip) {
          case STATUS_UPDATE:
            {
              uint8_t fans[6] = {0};
              uint8_t inputs[8] = {0};
              float sensors[5] = {0.0};
              uint32_t offTimes[6] = {0};
              float currentPower = 0.0;
              float energyConsumption = 0.0;
              uint8_t errorFlags[5] = {0};

              memcpy(fans, &buffer[2], 6);
              memcpy(inputs, &buffer[8], 8);
              memcpy(sensors, &buffer[16], 5 * sizeof(float));
              memcpy(offTimes, &buffer[36], 6 * sizeof(uint32_t));
              memcpy(&currentPower, &buffer[60], sizeof(float));
              memcpy(&energyConsumption, &buffer[64], sizeof(float));
              memcpy(errorFlags, &buffer[68], 5);

              String logMsg = "[RS485] Parsed STATUS_UPDATE: Fans=[" + String(fans[0]) + "," + String(fans[1]) + "," + String(fans[2]) +
                              "," + String(fans[3]) + "," + String(fans[4]) + "," + String(fans[5]) + "]" +
                              ", Inputs=[" + String(inputs[0]) + "," + String(inputs[1]) + "," + String(inputs[2]) +
                              "," + String(inputs[3]) + "," + String(inputs[4]) + "," + String(inputs[5]) +
                              "," + String(inputs[6]) + "," + String(inputs[7]) + "]" +
                              ", Sensors=[T_kop=" + String(sensors[0], 2) + ", H_kop=" + String(sensors[1], 2) +
                              ", P_kop=" + String(sensors[2], 2) + ", T_ut=" + String(sensors[3], 2) +
                              ", H_ut=" + String(sensors[4], 2) + "]" +
                              ", Power=" + String(currentPower, 2) + ", Energy=" + String(energyConsumption, 2) +
                              ", ErrorFlags=[" + String(errorFlags[0]) + "," + String(errorFlags[1]) + "," +
                              String(errorFlags[2]) + "," + String(errorFlags[3]) + "," + String(errorFlags[4]) + "]" +
                              ", OffTimes=[" + String(offTimes[0]) + "," + String(offTimes[1]) + "," + String(offTimes[2]) +
                              "," + String(offTimes[3]) + "," + String(offTimes[4]) + "," + String(offTimes[5]) + "]";
              Serial.println(logMsg);

              // Posodobitev sensorData in needsRedraw (enako kot prej)
              bool kopChanged = (sensorData.kopTemp != sensors[0] || sensorData.kopHumidity != sensors[1] || sensorData.kopPressure != sensors[2]);
              bool utChanged = (sensorData.utTemp != sensors[3] || sensorData.utHumidity != sensors[4]);
              bool powerChanged = (sensorData.currentPower != currentPower || sensorData.energyConsumption != energyConsumption);
              bool errorChanged = false;
              for (int j = 0; j < 5; j++) {
                if (sensorData.errorFlags[j] != errorFlags[j]) errorChanged = true;
              }

              sensorData.prevKopTemp = sensorData.kopTemp;
              sensorData.prevKopHumidity = sensorData.kopHumidity;
              sensorData.prevKopPressure = sensorData.kopPressure;
              sensorData.prevUtTemp = sensorData.utTemp;
              sensorData.prevUtHumidity = sensorData.utHumidity;
              sensorData.prevCurrentPower = sensorData.currentPower;
              sensorData.prevEnergyConsumption = sensorData.energyConsumption;
              sensorData.kopTemp = sensors[0];
              sensorData.kopHumidity = sensors[1];
              sensorData.kopPressure = sensors[2];
              sensorData.utTemp = sensors[3];
              sensorData.utHumidity = sensors[4];
              sensorData.currentPower = currentPower;
              sensorData.energyConsumption = energyConsumption;
              memcpy(sensorData.errorFlags, errorFlags, 5);
              memcpy(sensorData.fans, fans, 6);
              memcpy(sensorData.inputs, inputs, 8);
              memcpy(sensorData.offTimes, offTimes, 6 * sizeof(uint32_t));

              if (kopChanged) {
                needsRedraw[KOP] = true;
                needsRedraw[WC] = true;
              }
              if (utChanged) needsRedraw[UT] = true;
              if (powerChanged || errorChanged) needsRedraw[TIME_WIFI] = true;

              int roomMap[] = {0, 1, 0, 2, 3};
              for (int i = 1; i <= 4; i++) {
                int rId = roomMap[i];
                bool newFanActive = (i == 4) ? (fans[4] || fans[5]) : fans[rId];
                unsigned long newRemaining = (offTimes[rId] > myTZ.now()) ? offTimes[rId] - myTZ.now() : 0;
                newRemaining = std::min(newRemaining, 999UL);
                bool newLightOn = false;
                if (i == 2) newLightOn = inputs[4] || inputs[5];
                else if (i == 1) newLightOn = inputs[6];
                else if (i == 3) newLightOn = inputs[7];

                if (newFanActive != squares[i].fanActive || newRemaining != squares[i].fanRemainingTime || newLightOn != squares[i].lightOn) {
                  squares[i].fanActive = newFanActive;
                  squares[i].fanRemainingTime = newRemaining;
                  squares[i].lightOn = newLightOn;
                  needsRedraw[i] = true;
                }
              }

              confirmData[1] = STATUS_UPDATE;
              sendViaRS485(confirmData, sizeof(confirmData));
              waitingResponse = false;
              retryRequestCount = 0;
              dnTriggered = false;

              processStatusUpdate();
            }
            break;

          case CONFIRM_ACTION:
            {
              uint8_t messageID = buffer[2];
              uint8_t context = buffer[3];
              Serial.println("[RS485] Parsed CONFIRM_ACTION: ID=" + String(messageID) + ", Kontekst=" + String(context));
              if (messageID == MANUAL_CONTROL) {
                Serial.println("[RS485] MANUAL_CONTROL potrditev uspešna");
                waitingForManual = false;
              } else if (messageID == PARAM_UPDATE) {
                EEPROM.put(0, newSettings);
                EEPROM.commit();
                Serial.println("[RS485] EEPROM shranjen po potrditvi PARAM_UPDATE");
                waitingForConfirmAction = false;
              }
            }
            break;

          case HISTORY_RESPONSE:
            {
              uint32_t date = (buffer[2] << 24) | (buffer[3] << 16) | (buffer[4] << 8) | buffer[5];
              uint16_t zipSize = (buffer[6] << 8) | buffer[7];
              memcpy(historyBuffer, &buffer[8], zipSize);
              historyBufferSize = zipSize;
              Serial.println("[RS485] Parsed HISTORY_RESPONSE: Datum=" + String(date) + ", Velikost=" + String(zipSize) + " bajtov");

              confirmData[1] = HISTORY_RESPONSE;
              sendViaRS485(confirmData, sizeof(confirmData));
              waitingForConfirmAction = false;
            }
            break;

          case GRAPH_DATA:
            {
              uint8_t graphType = buffer[2];
              uint32_t timestamp = (buffer[3] << 24) | (buffer[4] << 16) | (buffer[5] << 8) | buffer[6];
              uint8_t* dataPtr = &buffer[7];
              switch (graphType) {
                case static_cast<uint8_t>(GraphType::TEMP):
                  memcpy(temperatureGraph, dataPtr, 4 * GRAPH_POINTS);
                  break;
                case static_cast<uint8_t>(GraphType::HUM):
                  memcpy(humidityGraph, dataPtr, 4 * GRAPH_POINTS);
                  break;
                case static_cast<uint8_t>(GraphType::CO2LIGHT):
                  memcpy(co2LightGraph, dataPtr, 2 * GRAPH_POINTS);
                  break;
                case static_cast<uint8_t>(GraphType::FAN):
                  memcpy(fanGraph, dataPtr, 4 * GRAPH_POINTS);
                  break;
              }
              Serial.println("[RS485] Parsed GRAPH_DATA: Tip=" + String(graphType) + ", Timestamp=" + String(timestamp));
              waitingForConfirmAction = false;
            }
            break;

          default:
            Serial.println("[RS485] Nepoznan tip sporočila: " + String(buffer[1]));
            break;
        }
        bufferIndex = 0;
        foundStartMarker = false;
      }
      vTaskDelay(1);
    }
  }
}

void sendViaRS485(uint8_t* data, size_t len) {
  Serial2.write(0x7E);
  uint16_t crc = calcCRC16(data, len, 0x1021);
  Serial2.write(data, len);
  Serial2.write((uint8_t)(crc >> 8));
  Serial2.write((uint8_t)(crc & 0xFF));
  Serial2.write('\n');
  String hexDump = "";
  for (size_t i = 0; i < len; i++) {
    hexDump += String(data[i], HEX) + " ";
  }
  Serial.println("[RS485] Pošiljam sporočilo (surovi bajti HEX): 7e " + hexDump);
}

void sendSensorData() {
  uint8_t data[30];
  data[0] = SENSOR_DATA;
  memcpy(&data[1], &sensorData.extTemp, sizeof(float));
  memcpy(&data[5], &sensorData.extHumidity, sizeof(float));
  memcpy(&data[9], &sensorData.extPressure, sizeof(float));
  memcpy(&data[13], &sensorData.localTemp, sizeof(float));
  memcpy(&data[17], &sensorData.localHumidity, sizeof(float));
  memcpy(&data[21], &sensorData.localCO2, sizeof(float));
  uint32_t timestamp = myTZ.now();
  memcpy(&data[25], &timestamp, sizeof(uint32_t));
  sendViaRS485(data, 29);
  Serial.println("[RS485] SENSOR_DATA poslan: Čas=" + String(timestamp));
}

void sendManualControl(uint8_t roomID, uint8_t command) {
  uint8_t data[3];
  data[0] = MANUAL_CONTROL;
  data[1] = roomID;
  data[2] = command;
  sendViaRS485(data, 3);
  waitingForManual = true;
  lastManualSent = millis();
  Serial.println("[RS485] MANUAL_CONTROL poslan: RoomID=" + String(roomID) + ", Ukaz=" + String(command));
}

void sendGraphRequest(GraphType graphType) {
  uint8_t data[2];
  data[0] = GRAPH_REQUEST;
  data[1] = static_cast<uint8_t>(graphType);
  sendViaRS485(data, 2);
  waitingForConfirmAction = true;
  lastRequestSent = millis();
  retryRequestCount = 0;
  Serial.println("[RS485] GRAPH_REQUEST poslan: Tip=" + String(static_cast<int>(graphType)));
}

void sendHistoryRequest(uint32_t date) {
  uint8_t data[5];
  data[0] = HISTORY_REQUEST;
  memcpy(&data[1], &date, sizeof(uint32_t));
  sendViaRS485(data, 5);
  waitingForConfirmAction = true;
  lastRequestSent = millis();
  retryRequestCount = 0;
  Serial.println("[RS485] HISTORY_REQUEST poslan: Datum=" + String(date));
}

void sendParamUpdate() {
  uint8_t data[sizeof(Settings)];
  data[0] = PARAM_UPDATE;
  memcpy(&data[1], &newSettings, sizeof(Settings) - 1);
  sendViaRS485(data, sizeof(Settings));
  waitingForConfirmAction = true;
  lastParamUpdateSent = millis();
  retryRequestCount = 0;
  Serial.println("[RS485] PARAM_UPDATE poslan");
}

void sendStateRequest() {
  uint8_t data[1];
  data[0] = STATE_REQUEST;
  sendViaRS485(data, 1);
  waitingResponse = true;
  lastRequestSent = millis();
  retryRequestCount = 0;
  Serial.println("[RS485] STATE_REQUEST poslan");
}
// src/485.cpp