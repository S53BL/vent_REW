// sens.cpp - Sensor module implementation
#include "sens.h"
#include "globals.h"
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include <SensirionI2cScd4x.h>

// Sensor objects
Adafruit_SHT4x *sht41 = nullptr;
SensirionI2cScd4x *scd40 = nullptr;

bool checkI2CDevice(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

bool initSensors() {
    // Set I2C timeout
    Wire.setTimeout(I2C_TIMEOUT_MS);

    // Initialize SHT41
    for (int retry = 0; retry < 3; retry++) {
      if (checkI2CDevice(0x44)) {
        sht41 = new Adafruit_SHT4x();
        if (sht41->begin()) {
          sht41->setPrecision(SHT4X_HIGH_PRECISION);
          sht41->setHeater(SHT4X_NO_HEATER);
          Serial.println("[Sensor] SHT41 initialized");
          Serial.flush();
          break;
        } else {
          Serial.println("[Sensor] SHT41 begin failed");
          Serial.flush();
          delete sht41;
          sht41 = nullptr;
        }
      } else {
        Serial.println("[Sensor] SHT41 not detected at 0x44");
        Serial.flush();
      }
      delay(200);
    }
    if (!sht41) {
      sensorData.errorFlags[0] |= ERR_SENSOR;
    }

    // Initialize SCD40
    for (int retry = 0; retry < 3; retry++) {
      if (checkI2CDevice(0x62)) {
        scd40 = new SensirionI2cScd4x();
        scd40->begin(Wire, 0x62);
        uint16_t scdError = scd40->stopPeriodicMeasurement();
        if (!scdError) {
          scdError = scd40->startPeriodicMeasurement();
          if (!scdError) {
            Serial.println("[Sensor] SCD40 initialized");
            Serial.flush();
            break;
          } else {
            Serial.println("[Sensor] SCD40 startPeriodicMeasurement failed");
            Serial.flush();
          }
        } else {
          Serial.println("[Sensor] SCD40 stopPeriodicMeasurement failed");
          Serial.flush();
        }
        delete scd40;
        scd40 = nullptr;
      } else {
        Serial.println("[Sensor] SCD40 not detected at 0x62");
        Serial.flush();
      }
      delay(200);
    }
    if (!scd40) {
      sensorData.errorFlags[0] |= ERR_SENSOR;
    }

    return (sensorData.errorFlags[0] & ERR_SENSOR) == 0;
}

void readSensors() {
    Serial.println("Sensor periodic read called");
    Serial.flush();

    if (Wire.available()) return; // Avoid conflicts

    bool updateUI = false;

    // Read SHT41
    if (checkI2CDevice(0x44)) {
        sensors_event_t humidity, temp;
        if (sht41->getEvent(&humidity, &temp)) {
            float newTemp = temp.temperature;
            float newHum = humidity.relative_humidity;

            // Validate values
            if (isnan(newTemp) || newTemp < -40.0f || newTemp > 125.0f) {
                Serial.printf("[SHT41] Invalid temperature: %.2f\n", newTemp);
            } else if (isnan(newHum) || newHum < 0.0f || newHum > 100.0f) {
                Serial.printf("[SHT41] Invalid humidity: %.2f\n", newHum);
            } else {
                // Always update values (no threshold checking)
                sensorData.localTemp = newTemp;
                sensorData.localHumidity = newHum;
                sensorData.lastTemp = newTemp;
                sensorData.lastHumidity = newHum;
                updateUI = true;
                Serial.printf("[Sensor] SHT41 read: temp=%.1f, humidity=%.1f\n", newTemp, newHum);
            }
        }
    } else {
        Serial.println("[Sensor] SHT41 not detected - skipping read");
    }

    // Read SCD40
    if (checkI2CDevice(0x62)) {
        uint16_t co2;
        float temperature, hum;
        bool dataReady = false;
        uint16_t error = scd40->getDataReadyStatus(dataReady);
        if (!error && dataReady) {
            error = scd40->readMeasurement(co2, temperature, hum);
            if (!error) {
                float newCO2 = (float)co2;

                // Validate value
                if (isnan(newCO2) || newCO2 < 400.0f || newCO2 > 5000.0f) {
                    Serial.printf("[SCD40] Invalid CO2: %.0f\n", newCO2);
                } else {
                    // Always update values (no threshold checking)
                    sensorData.localCO2 = newCO2;
                    sensorData.lastCO2 = newCO2;
                    updateUI = true;
                    Serial.printf("[Sensor] SCD40 read: CO2=%d\n", co2);
                }
            } else {
                Serial.println("[SCD40] Read measurement failed");
            }
        } else {
            Serial.println("[Sensor] SCD40 data not ready - skipping read");
        }
    } else {
        Serial.println("[Sensor] SCD40 not detected - skipping read");
    }

    // Update UI if any sensor changed
    if (updateUI) {
        extern void updateUI(); // Declare extern to avoid including disp.h
        updateUI();
    }
}

float getTemperature() {
    return sensorData.localTemp;
}

float getHumidity() {
    return sensorData.localHumidity;
}

float getCO2() {
    return sensorData.localCO2;
}

void resetSensors() {
    Wire.end();
    initSensors();
}
