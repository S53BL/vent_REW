// sd.cpp - SD card module implementation
#include "sd.h"
#include "globals.h"

bool initSD() {
    SD_MMC.setPins(14,17,16);
    if (!SD_MMC.begin("/sdcard", true)) {
        sensorData.errorFlags[0] |= ERR_SD;
        Serial.println("SD init failed - check pins");
        return false;
    }
    return true;
}

void saveHistorySens() {
  if (sensorData.errorFlags[0] & ERR_SD) {
    Serial.println("SD ERR");
    return;
  }
  String currentDate = myTZ.dateTime("Y-m-d");
  if (currentDate != lastDate) {
    lastDate = currentDate;
    currentSensFile = String("/history_sens_") + myTZ.dateTime("Ymd") + String(".csv");
    File file = SD_MMC.open(currentSensFile.c_str(), FILE_WRITE);
    if (!file) {
      Serial.println("SD open fail");
      return;
    }
    file.println("Čas zapisa,Temperatura zunaj,Vlaga zunaj,Tlak zunaj,VOC zunaj,Svetloba zunaj,Temperatura DS,Vlaga DS,CO2 DS,Temperatura UT,Vlaga UT,Temperatura KOP,Vlaga KOP,Tlak WC,Vremenski code");
    file.close();
  }
  File file = SD_MMC.open(currentSensFile.c_str(), FILE_APPEND);
  if (!file) {
    Serial.println("SD open fail");
    return;
  }
  char line[256];
  String timeStr = myTZ.dateTime("H:i:s d.m.y");
  if (!timeSynced) {
    timeStr = String("millis:") + String(millis());
  }
  sprintf(line, "%s,%.1f,%.1f,%d,%d,%d,%.1f,%.1f,%d,%.1f,%.1f,%.1f,%.1f,%d,%d",
          timeStr.c_str(),
          sensorData.extTemp,
          sensorData.extHumidity,
          (int)sensorData.extPressure,
          sensorData.extVOC,
          sensorData.extLux,
          sensorData.localTemp,
          sensorData.localHumidity,
          (int)sensorData.localCO2,
          sensorData.utTemp,
          sensorData.utHumidity,
          sensorData.bathroomTemp,
          sensorData.bathroomHumidity,
          (int)sensorData.bathroomPressure,
          sensorData.weatherCode);
  file.println(line);
  file.close();
  Serial.println("Sens saved to " + currentSensFile);
}

void saveFanHistory() {
  if (sensorData.errorFlags[0] & ERR_SD) {
    Serial.println("SD ERR");
    return;
  }
  String currentDate = myTZ.dateTime("Y-m-d");
  if (currentDate != lastDate) {
    lastDate = currentDate;
    currentFanFile = String("/fan_history_") + myTZ.dateTime("Ymd") + String(".csv");
    File file = SD_MMC.open(currentFanFile.c_str(), FILE_WRITE);
    if (!file) {
      Serial.println("SD open fail");
      return;
    }
    file.println("Čas zapisa,WC stanje,UT stanje,KOP stanje,DS stanje");
    file.close();
  }
  File file = SD_MMC.open(currentFanFile.c_str(), FILE_APPEND);
  if (!file) {
    Serial.println("SD open fail");
    return;
  }
  char line[128];
  String timeStr = myTZ.dateTime("H:i:s d.m.y");
  String wcState = (sensorData.fanStates[0] == 1 ? "ON" : "OFF");
  String utState = (sensorData.fanStates[1] == 1 ? "ON" : "OFF");
  String kopState = (sensorData.fanStates[2] == 1 ? "ON" : "OFF");
  String dsState = (sensorData.fanStates[3] == 1 ? "ON" : "OFF");
  sprintf(line, "%s,%s,%s,%s,%s",
          timeStr.c_str(),
          wcState.c_str(),
          utState.c_str(),
          kopState.c_str(),
          dsState.c_str());
  file.println(line);
  file.close();
  Serial.println("Fan saved to " + currentFanFile);
}

void flushLogs() {
  Serial.println("Flushing logs...");
  /* placeholder za log flush */
}
