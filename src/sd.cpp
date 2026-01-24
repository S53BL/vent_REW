// sd.cpp - SD card module implementation
#include "sd.h"
#include "globals.h"
#include "logging.h"

bool initSD() {
    SD_MMC.setPins(14,17,16);
    if (!SD_MMC.begin("/sdcard", true)) {
        sensorData.errorFlags[0] |= ERR_SD;
        logEvent("SD:Init failed - check pins");
        return false;
    }
    return true;
}

void saveHistorySens() {
  if (sensorData.errorFlags[0] & ERR_SD) {
    logEvent("SD:ERR - cannot save sensor history");
    return;
  }
  String currentDate = myTZ.dateTime("Y-m-d");
  if (currentDate != lastDate) {
    lastDate = currentDate;
    currentSensFile = String("/history_sens_") + myTZ.dateTime("Ymd") + String(".csv");
    File file = SD_MMC.open(currentSensFile.c_str(), FILE_WRITE);
    if (!file) {
      logEvent("SD:Open fail for new sensor history file");
      return;
    }
    file.println("Čas zapisa,Temperatura zunaj,Vlaga zunaj,Tlak zunaj,VOC zunaj,Svetloba zunaj,Temperatura DS,Vlaga DS,CO2 DS,Temperatura UT,Vlaga UT,Temperatura KOP,Vlaga KOP,Tlak WC,Vremenski code");
    file.close();
  }
  File file = SD_MMC.open(currentSensFile.c_str(), FILE_APPEND);
  if (!file) {
    logEvent("SD:Open fail for sensor history append");
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
  logEvent("SD:Sens saved to " + currentSensFile);
}

void saveFanHistory() {
  if (sensorData.errorFlags[0] & ERR_SD) {
    logEvent("SD:ERR - cannot save fan history");
    return;
  }
  String currentDate = myTZ.dateTime("Y-m-d");
  if (currentDate != lastDate) {
    lastDate = currentDate;
    currentFanFile = String("/fan_history_") + myTZ.dateTime("Ymd") + String(".csv");
    File file = SD_MMC.open(currentFanFile.c_str(), FILE_WRITE);
    if (!file) {
      logEvent("SD:Open fail for new fan history file");
      return;
    }
    file.println("Čas zapisa,WC stanje,UT stanje,KOP stanje,DS stanje");
    file.close();
  }
  File file = SD_MMC.open(currentFanFile.c_str(), FILE_APPEND);
  if (!file) {
    logEvent("SD:Open fail for fan history append");
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
  logEvent("SD:Fan saved to " + currentFanFile);
}

void flushLogs() {
  logEvent("SD:Flushing logs");
  /* placeholder za log flush */
}

String readFile(const char* path) {
    File f = SD_MMC.open(path, FILE_READ);
    if (!f) return "";
    String s;
    while (f.available()) s += (char)f.read();
    f.close();
    return s;
}

String listFiles(const char* pattern, uint32_t from_date, uint32_t to_date) {
    String files = "";
    File root = SD_MMC.open("/");
    while (true) {
        File entry = root.openNextFile();
        if (!entry) break;
        String name = entry.name();
        if (strstr(name.c_str(), pattern)) {
            uint32_t fileDate = parseDateFromName(name);
            if (fileDate >= from_date && fileDate <= to_date) {
                if (files.length() > 0) files += ",";
                files += name;
            }
        }
        entry.close();
    }
    root.close();
    return files;
}

String listLogFiles(uint32_t from_date, uint32_t to_date) {
    return listFiles("logs_", from_date, to_date);
}

uint32_t parseDateFromName(String name) {
    int y = 0, m = 0, d = 0;
    if (sscanf(name.c_str(), "/history_sens_%4d%2d%2d.csv", &y, &m, &d) == 3 ||
        sscanf(name.c_str(), "/fan_history_%4d%2d%2d.csv", &y, &m, &d) == 3 ||
        sscanf(name.c_str(), "/logs_%4d%2d%2d.txt", &y, &m, &d) == 3) {
        return y * 10000 + m * 100 + d;
    }
    return 0;
}
