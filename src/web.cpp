#include "web.h"
#include "html.h"
#include "globals.h"
#include "sd.h"
#include <ESPAsyncWebServer.h>
extern AsyncWebServer server;

// Forward declaration for lambda functions
extern void logEvent(String content);

void setupWebEndpoints() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String statusContent = "EXT: Temp=" + String(sensorData.extTemp, 1) + "°C, Hum=" + String(sensorData.extHumidity, 1) + "%, Pres=" + String((int)sensorData.extPressure) + " hPa, Lux=" + String((int)sensorData.extLux) + " lx\n";
    statusContent += "DS: Temp=" + String(sensorData.localTemp, 1) + "°C, Hum=" + String(sensorData.localHumidity, 1) + "%, CO2=" + String((int)sensorData.localCO2) + " ppm\n";
    statusContent += "UT: Temp=" + String(sensorData.utTemp, 1) + "°C, Hum=" + String(sensorData.utHumidity, 1) + "%\n";
    statusContent += "KOP: Temp=" + String(sensorData.bathroomTemp, 1) + "°C, Hum=" + String(sensorData.bathroomHumidity, 1) + "%\n";
    statusContent += "WC: Pres=" + String((int)sensorData.bathroomPressure) + " hPa\n";
    statusContent += "TIME_WIFI: Power=" + String(sensorData.currentPower, 1) + " W, Energy=" + String(sensorData.energyConsumption, 1) + " Wh\n";
    statusContent += "Zadnja posodobitev: " + myTZ.dateTime("H:i:s d.m.y");

    if (sensorData.errorFlags[0] & ERR_SD) {
      statusContent += "\n<span class=\"error\">SD ni na voljo</span>";
    }

    char htmlBuffer[4096];
    snprintf(htmlBuffer, sizeof(htmlBuffer), HTML_ROOT, statusContent.c_str());
    request->send(200, "text/html", htmlBuffer);
  });
  Serial.println("WEB: Setup endpoint /");

  server.on("/help", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", HTML_HELP);
  });
  Serial.println("WEB: Setup endpoint /help");

  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request){
    if (sensorData.errorFlags[0] & ERR_SD) {
      request->send(503, "text/html", "<h1>SD ni na voljo</h1><a href='/'>Nazaj</a>");
      return;
    }

    String up_to = request->arg("up_to");
    String confirm = request->arg("confirm");

    if (up_to.length() > 0 && confirm == "yes") {
      // Delete files
      int y, m, d;
      if (sscanf(up_to.c_str(), "%d-%d-%d", &y, &m, &d) != 3) {
        request->send(400, "text/html", "<h1>Neveljaven datum</h1><a href='/delete'>Nazaj</a>");
        return;
      }
      uint32_t deleteBefore = y * 10000 + m * 100 + d;

      // Delete history_sens files
      String sensFiles = listFiles("history_sens_", 0, deleteBefore - 1);
      int deletedCount = 0;
      int start = 0;
      int fileCount = 0;
      while (start < sensFiles.length()) {
        int commaPos = sensFiles.indexOf(',', start);
        String fileName = (commaPos == -1) ? sensFiles.substring(start) : sensFiles.substring(start, commaPos);
        if (fileName.length() > 0 && SD_MMC.remove(fileName.c_str())) {
          deletedCount++;
        }
        start = (commaPos == -1) ? sensFiles.length() : commaPos + 1;
        fileCount++;
        if (fileCount % 10 == 0) yield(); // Yield every 10 files
      }

      // Delete fan_history files
      String fanFiles = listFiles("fan_history_", 0, deleteBefore - 1);
      start = 0;
      fileCount = 0;
      while (start < fanFiles.length()) {
        int commaPos = fanFiles.indexOf(',', start);
        String fileName = (commaPos == -1) ? fanFiles.substring(start) : fanFiles.substring(start, commaPos);
        if (fileName.length() > 0 && SD_MMC.remove(fileName.c_str())) {
          deletedCount++;
        }
        start = (commaPos == -1) ? fanFiles.length() : commaPos + 1;
        fileCount++;
        if (fileCount % 10 == 0) yield(); // Yield every 10 files
      }

      // Delete logs files (keep only last 7 days)
      String logFiles = listFiles("logs_", 0, deleteBefore - 1);
      start = 0;
      fileCount = 0;
      while (start < logFiles.length()) {
        int commaPos = logFiles.indexOf(',', start);
        String fileName = (commaPos == -1) ? logFiles.substring(start) : logFiles.substring(start, commaPos);
        if (fileName.length() > 0 && SD_MMC.remove(fileName.c_str())) {
          deletedCount++;
        }
        start = (commaPos == -1) ? logFiles.length() : commaPos + 1;
        fileCount++;
        if (fileCount % 10 == 0) yield(); // Yield every 10 files
      }

      logEvent("WEB: Delete completed, removed " + String(deletedCount) + " files before " + up_to);
      request->redirect("/?msg=Brisanje%20uspešno%20-%20izbrisanih%20" + String(deletedCount) + "%20datotek");
    } else if (up_to.length() > 0) {
      // Show confirmation
      char htmlBuffer[2048];
      snprintf(htmlBuffer, sizeof(htmlBuffer), HTML_DELETE_CONFIRM, up_to.c_str(), up_to.c_str());
      request->send(200, "text/html", htmlBuffer);
    } else {
      // Show form
      char htmlBuffer[1024];
      snprintf(htmlBuffer, sizeof(htmlBuffer), HTML_DELETE_FORM, myTZ.dateTime("Y-m-d").c_str());
      request->send(200, "text/html", htmlBuffer);
    }
  });
  Serial.println("WEB: Setup endpoint /delete");

  // Comment out complex handlers for hang diagnosis
  /*
  server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request){
    // ... complex logs handler commented out
  });

  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request){
    // ... complex history handler commented out
  });

  server.on("/history/download", HTTP_GET, [](AsyncWebServerRequest *request){
    // ... complex history download handler commented out
  });

  server.on("/logs/export", HTTP_GET, [](AsyncWebServerRequest *request){
    // ... complex logs export handler commented out
  });
  */

  server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request){
    if (sensorData.errorFlags[0] & ERR_SD) {
      request->send(503, "text/html", "<h1>SD ni na voljo</h1><a href='/'>Nazaj</a>");
      return;
    }

    Serial.printf("WEB: Starting logs request, free heap: %u\n", ESP.getFreeHeap());

    // Parse parameters with defaults
    String dateStr = request->arg("date");
    String timeStr = request->arg("time");

    if (dateStr.length() == 0) {
      dateStr = myTZ.dateTime("Y-m-d");
    }
    if (timeStr.length() == 0) {
      // Round down to current hour
      int currentHour = myTZ.dateTime("H").toInt();
      char hourStr[3];
      sprintf(hourStr, "%02d:00", currentHour);
      timeStr = String(hourStr);
    }

    // Calculate unix timestamps for 1-hour window
    uint32_t from_unix, to_unix;
    String dateTimeStr = dateStr + " " + timeStr + ":00";
    // Parse date and time manually since ezTime dateTime() returns String
    int y, m, d, h, min;
    if (sscanf(dateTimeStr.c_str(), "%d-%d-%d %d:%d:00", &y, &m, &d, &h, &min) == 5) {
      // Create time structure
      struct tm timeinfo = {0};
      timeinfo.tm_year = y - 1900;
      timeinfo.tm_mon = m - 1;
      timeinfo.tm_mday = d;
      timeinfo.tm_hour = h;
      timeinfo.tm_min = min;
      timeinfo.tm_sec = 0;

      // Convert to unix timestamp (assuming local time)
      time_t local_time = mktime(&timeinfo);
      to_unix = local_time;
      from_unix = to_unix - 3600;
    } else {
      // Fallback to current time
      uint32_t current_time = myTZ.now();
      to_unix = current_time - (current_time % 3600); // Round down to hour
      from_unix = to_unix - 3600;
    }

    // List log files for last 7 days
    uint32_t sevenDaysAgo = myTZ.now() - (7 * 24 * 3600);
    uint32_t today = myTZ.now();
    String logFiles = listLogFiles(sevenDaysAgo, today);

    // Reverse file order for descending chronological processing (newest first)
    std::reverse(logFiles.begin(), logFiles.end());

    // Use PSRAM for large strings if available
    String tableHtml = "";
    if (ESP.getPsramSize() > 0) {
      tableHtml.reserve(5000); // Reduced from 10000 to prevent low heap
      Serial.printf("WEB: Using PSRAM, size: %u\n", ESP.getPsramSize());
    }

    tableHtml += "<div class=\"scrollable\"><table><thead><tr><th>Čas (lokalni)</th><th>Enota</th><th>Sporočilo</th></tr></thead><tbody>";

    int count = 0;

    // Split file list and read each file (stream processing)
    int fileStart = 0;
    while (fileStart < logFiles.length() && count < MAX_ROWS) {
      int commaPos = logFiles.indexOf(',', fileStart);
      String fileName = (commaPos == -1) ? logFiles.substring(fileStart) : logFiles.substring(fileStart, commaPos);

      if (fileName.length() > 0) {
        Serial.printf("WEB: Processing file %s\n", fileName.c_str());
        File f = SD_MMC.open(fileName.c_str(), FILE_READ);
        if (f) {
          while (f.available() && count < MAX_ROWS) {
            // Safe readStringUntil with timeout/loop to prevent hangs
            String line = "";
            int charCount = 0;
            while (f.available() && charCount < 1024) {
              char c = f.read();
              if (c == '\n') break;
              line += c;
              charCount++;
            }
            if (charCount >= 1024) {
              Serial.println("WEB: Line too long, skipping");
              continue; // Skip overly long lines
            }
            if (!f.available() && line.length() == 0) break; // No more data

            line.trim();
            Serial.print(".");

            if (line.length() > 0) {
              // Parse unix|unit|message
              int pipe1 = line.indexOf('|');
              int pipe2 = line.indexOf('|', pipe1 + 1);

              if (pipe1 > 0 && pipe2 > pipe1) {
                String unixStr = line.substring(0, pipe1);
                String unitStr = line.substring(pipe1 + 1, pipe2);
                String msgStr = line.substring(pipe2 + 1);

                uint32_t unix_time = unixStr.toInt();
                if (unix_time >= from_unix && unix_time <= to_unix) {
                  // Determine row class based on message content
                  String rowClass = "row-c"; // default green
                  if (msgStr.indexOf("ERR") >= 0 || msgStr.indexOf("failed") >= 0) {
                    rowClass = "row-r"; // red for errors
                  }

                  String localTime = myTZ.dateTime(unix_time, "H:i:s d.m.y");
                  tableHtml += "<tr class=\"" + rowClass + "\"><td>" + localTime + "</td><td>" + unitStr + "</td><td>" + msgStr + "</td></tr>";
                  count++;
                }
              }
            }
          }
          f.close();
        } else {
          Serial.printf("WEB: Failed to open file: %s\n", fileName.c_str());
        }
      }

      fileStart = (commaPos == -1) ? logFiles.length() : commaPos + 1;
    }

    tableHtml += "</tbody></table></div>";

    if (count == 0) {
      tableHtml = "<p>Ni podatkov za izbrano obdobje.</p>";
    } else if (count >= MAX_ROWS) {
      tableHtml += "<div class=\"warning\">Prikaz omejen na " + String(MAX_ROWS) + " vrstic; za polne podatke uporabi izvoz.</div>";
    }

    Serial.printf("WEB: Finished, rows: %d, heap: %u\n", count, ESP.getFreeHeap());
    if (ESP.getPsramSize() > 0) {
      Serial.printf("WEB: PSRAM free: %u\n", ESP.getFreePsram());
    }

    logEvent("WEB: Logs request - rows processed: " + String(count));

    // Send response
    char htmlBuffer[8192];
    snprintf(htmlBuffer, sizeof(htmlBuffer), HTML_LOGS_FORM, dateStr.c_str(), timeStr.c_str(), tableHtml.c_str());
    request->send(200, "text/html", htmlBuffer);
  });

  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request){
    if (sensorData.errorFlags[0] & ERR_SD) {
      request->send(503, "text/html", "<h1>SD ni na voljo</h1><a href='/'>Nazaj</a>");
      return;
    }

    // Parse parameters with defaults
    String fromStr = request->arg("from");
    String toStr = request->arg("to");
    String typeStr = request->arg("type");

    if (fromStr.length() == 0) {
      // Default to yesterday
      uint32_t yesterday = myTZ.now() - 86400;
      fromStr = myTZ.dateTime(yesterday, "Y-m-d");
    }
    if (toStr.length() == 0) {
      toStr = myTZ.dateTime("Y-m-d");
    }
    if (typeStr.length() == 0) {
      typeStr = "all";
    }

    // Convert dates to YYYYMMDD
    uint32_t fromDate = 0, toDate = 0;
    int y, m, d;
    if (sscanf(fromStr.c_str(), "%d-%d-%d", &y, &m, &d) == 3) {
      fromDate = y * 10000 + m * 100 + d;
    }
    if (sscanf(toStr.c_str(), "%d-%d-%d", &y, &m, &d) == 3) {
      toDate = y * 10000 + m * 100 + d;
    }

    // Use PSRAM for large strings if available
    String table_html_sens = "";
    String table_html_vent = "";
    int count = 0;
    bool limit_reached = false;

    if (ESP.getPsramSize() > 0) {
      table_html_sens.reserve(10000);
      table_html_vent.reserve(10000);
    }

    if (typeStr == "sens" || typeStr == "all") {
      // Sensor data - stream processing
      String sensFiles = listFiles("history_sens_", fromDate, toDate);

      if (sensFiles.length() > 0) {
        table_html_sens += "<table><thead><tr><th>Čas</th><th>Ext Temp</th><th>Ext Hum</th><th>Ext Pres</th><th>Ext Lux</th><th>DS Temp</th><th>DS Hum</th><th>DS CO2</th><th>Kop Temp</th><th>Kop Hum</th><th>Kop Pres</th><th>Ut Temp</th><th>Ut Hum</th><th>Wc Pres</th><th>Rezerva</th></tr></thead><tbody>";

        int fileStart = 0;
        while (fileStart < sensFiles.length() && count < MAX_ROWS) {
          int commaPos = sensFiles.indexOf(',', fileStart);
          String fileName = (commaPos == -1) ? sensFiles.substring(fileStart) : sensFiles.substring(fileStart, commaPos);

          if (fileName.length() > 0) {
            File f = SD_MMC.open(fileName.c_str(), FILE_READ);
            if (f) {
              // Skip header
              f.readStringUntil('\n');

              while (f.available() && count < MAX_ROWS) {
                String line = f.readStringUntil('\n');
                line.trim();

                if (line.length() > 0) {
                  table_html_sens += "<tr>";
                  int col = 0;
                  int start = 0;
                  while (start < line.length() && col < 15) {
                    int comma = line.indexOf(',', start);
                    String field = (comma == -1) ? line.substring(start) : line.substring(start, comma);
                    table_html_sens += "<td>" + field + "</td>";
                    start = (comma == -1) ? line.length() : comma + 1;
                    col++;
                  }
                  table_html_sens += "</tr>";
                  count++;
                }
              }
              f.close();
            }
          }

          fileStart = (commaPos == -1) ? sensFiles.length() : commaPos + 1;
        }

        table_html_sens += "</tbody></table>";
      }
    }

    if (typeStr == "vent" || typeStr == "all") {
      // Fan data - stream processing
      String fanFiles = listFiles("fan_history_", fromDate, toDate);

      if (fanFiles.length() > 0) {
        table_html_vent += "<table><thead><tr><th>Čas</th><th>Kop Fan</th><th>Ut Fan</th><th>Wc Fan</th><th>Common Intake</th></tr></thead><tbody>";

        int fileStart = 0;
        while (fileStart < fanFiles.length() && count < MAX_ROWS) {
          int commaPos = fanFiles.indexOf(',', fileStart);
          String fileName = (commaPos == -1) ? fanFiles.substring(fileStart) : fanFiles.substring(fileStart, commaPos);

          if (fileName.length() > 0) {
            File f = SD_MMC.open(fileName.c_str(), FILE_READ);
            if (f) {
              // Skip header
              f.readStringUntil('\n');

              while (f.available() && count < MAX_ROWS) {
                String line = f.readStringUntil('\n');
                line.trim();

                if (line.length() > 0) {
                  table_html_vent += "<tr>";
                  int col = 0;
                  int start = 0;
                  while (start < line.length() && col < 5) {
                    int comma = line.indexOf(',', start);
                    String field = (comma == -1) ? line.substring(start) : line.substring(start, comma);
                    table_html_vent += "<td>" + field + "</td>";
                    start = (comma == -1) ? line.length() : comma + 1;
                    col++;
                  }
                  table_html_vent += "</tr>";
                  count++;
                }
              }
              f.close();
            }
          }

          fileStart = (commaPos == -1) ? fanFiles.length() : commaPos + 1;
        }

        table_html_vent += "</tbody></table>";
      }
    }

    // Build final tableHtml
    String tableHtml = "";
    if (typeStr == "all") {
      if (table_html_sens.length() > 0) {
        tableHtml += "<div class=\"table-title\">Senzorji</div>" + table_html_sens;
      }
      if (table_html_vent.length() > 0) {
        tableHtml += "<div class=\"table-title\">Ventilatorji</div>" + table_html_vent;
      }
    } else if (typeStr == "sens") {
      tableHtml = table_html_sens;
    } else if (typeStr == "vent") {
      tableHtml = table_html_vent;
    }

    if (tableHtml.length() == 0) {
      tableHtml = "<p>Ni podatkov za izbrano obdobje.</p>";
    } else if (count >= MAX_ROWS) {
      tableHtml += "<div class=\"warning\">Prikaz omejen na " + String(MAX_ROWS) + " vrstic; za polne podatke uporabi izvoz.</div>";
    }

    logEvent("WEB: History request - rows processed: " + String(count));

    // Send response
    char htmlBuffer[16384];
    snprintf(htmlBuffer, sizeof(htmlBuffer), HTML_HISTORY_FORM, fromStr.c_str(), toStr.c_str(), tableHtml.c_str());
    request->send(200, "text/html", htmlBuffer);
  });

  server.on("/history/download", HTTP_GET, [](AsyncWebServerRequest *request){
    if (sensorData.errorFlags[0] & ERR_SD) {
      request->send(503, "text/plain", "SD ni na voljo");
      return;
    }

    // Parse required parameters
    String typeStr = request->arg("type");
    String fromStr = request->arg("from");
    String toStr = request->arg("to");

    if (typeStr.length() == 0 || fromStr.length() == 0 || toStr.length() == 0) {
      request->send(400, "text/plain", "Manjkajo parametri: type, from, to");
      return;
    }

    if (typeStr == "all") {
      request->send(400, "text/plain", "Izberi specifičen type za izvoz (sens ali vent)");
      return;
    }

    if (typeStr != "sens" && typeStr != "vent") {
      request->send(400, "text/plain", "Neveljaven type: uporabi 'sens' ali 'vent'");
      return;
    }

    // Convert dates to YYYYMMDD
    uint32_t fromDate = 0, toDate = 0;
    int y, m, d;
    if (sscanf(fromStr.c_str(), "%d-%d-%d", &y, &m, &d) == 3) {
      fromDate = y * 10000 + m * 100 + d;
    }
    if (sscanf(toStr.c_str(), "%d-%d-%d", &y, &m, &d) == 3) {
      toDate = y * 10000 + m * 100 + d;
    }

    // List files
    const char* pattern = (typeStr == "sens") ? "history_sens_" : "fan_history_";
    String files = listFiles(pattern, fromDate, toDate);

    if (files.length() == 0) {
      request->send(404, "text/plain", "Ni podatkov za izbrano obdobje");
      return;
    }

    // Build CSV content directly (stream processing)
    String csvContent;
    if (typeStr == "sens") {
      csvContent = "Čas zapisa,Temperatura zunaj,Vlaga zunaj,Tlak zunaj,VOC zunaj,Svetloba zunaj,Temperatura DS,Vlaga DS,CO2 DS,Temperatura UT,Vlaga UT,Temperatura KOP,Vlaga KOP,Tlak WC,Vremenski code\n";
    } else {
      csvContent = "Čas zapisa,WC stanje,UT stanje,KOP stanje,DS stanje\n";
    }

    if (ESP.getPsramSize() > 0) {
      csvContent.reserve(10000);
    }

    int count = 0;

    // Parse files directly to CSV (no vector storage)
    int fileStart = 0;
    while (fileStart < files.length()) {
      int commaPos = files.indexOf(',', fileStart);
      String fileName = (commaPos == -1) ? files.substring(fileStart) : files.substring(fileStart, commaPos);

      if (fileName.length() > 0) {
        Serial.printf("WEB: Processing file %s\n", fileName.c_str());
        File f = SD_MMC.open(fileName.c_str(), FILE_READ);
        if (f) {
          // Skip header
          f.readStringUntil('\n');

          while (f.available()) {
            String line = f.readStringUntil('\n');
            line.trim();

            if (line.length() > 0) {
              // Parse CSV line and append directly to csvContent
              int colStart = 0;
              int colCount = 0;
              while (colStart < line.length()) {
                int commaPos = line.indexOf(',', colStart);
                String col = (commaPos == -1) ? line.substring(colStart) : line.substring(colStart, commaPos);
                if (colCount > 0) csvContent += ",";
                csvContent += col;
                colStart = (commaPos == -1) ? line.length() : commaPos + 1;
                colCount++;
              }
              csvContent += "\n";
              count++;
            }
          }
          f.close();
        } else {
          Serial.printf("WEB: Failed to open file: %s\n", fileName.c_str());
        }
      }

      fileStart = (commaPos == -1) ? files.length() : commaPos + 1;
    }

    if (count == 0) {
      request->send(404, "text/plain", "Ni podatkov za izbrano obdobje");
      return;
    }

    logEvent("WEB: Download /history/" + typeStr + " from " + fromStr + " to " + toStr + ", " + String(count) + " rows");

    // Send CSV response
    AsyncWebServerResponse *response = request->beginResponse(200, "text/csv", csvContent);
    String filename = "history_" + typeStr + ".csv";
    String disposition = "attachment; filename=" + filename;
    response->addHeader("Content-Disposition", disposition.c_str());
    request->send(response);
  });

  server.on("/logs/export", HTTP_GET, [](AsyncWebServerRequest *request){
    if (sensorData.errorFlags[0] & ERR_SD) {
      request->send(503, "text/plain", "SD ni na voljo");
      return;
    }

    // Parse parameters with defaults
    String dateStr = request->arg("date");
    String timeStr = request->arg("time");

    if (dateStr.length() == 0) {
      dateStr = myTZ.dateTime("Y-m-d");
    }
    if (timeStr.length() == 0) {
      // Round down to current hour
      int currentHour = myTZ.dateTime("H").toInt();
      char hourStr[3];
      sprintf(hourStr, "%02d:00", currentHour);
      timeStr = String(hourStr);
    }

    // Calculate unix timestamps for 1-hour window
    uint32_t from_unix, to_unix;
    String dateTimeStr = dateStr + " " + timeStr + ":00";
    // Parse date and time manually since ezTime dateTime() returns String
    int y, m, d, h, min;
    if (sscanf(dateTimeStr.c_str(), "%d-%d-%d %d:%d:00", &y, &m, &d, &h, &min) == 5) {
      // Create time structure
      struct tm timeinfo = {0};
      timeinfo.tm_year = y - 1900;
      timeinfo.tm_mon = m - 1;
      timeinfo.tm_mday = d;
      timeinfo.tm_hour = h;
      timeinfo.tm_min = min;
      timeinfo.tm_sec = 0;

      // Convert to unix timestamp (assuming local time)
      time_t local_time = mktime(&timeinfo);
      to_unix = local_time;
      from_unix = to_unix - 3600;
    } else {
      // Fallback to current time
      uint32_t current_time = myTZ.now();
      to_unix = current_time - (current_time % 3600); // Round down to hour
      from_unix = to_unix - 3600;
    }

    // List log files for last 7 days
    uint32_t sevenDaysAgo = myTZ.now() - (7 * 24 * 3600);
    uint32_t today = myTZ.now();
    String logFiles = listLogFiles(sevenDaysAgo, today);

    // Reverse file order for descending chronological processing (newest first)
    std::reverse(logFiles.begin(), logFiles.end());

    // Use PSRAM for large strings if available
    String csvContent = "Čas (lokalni),Unix čas,Enota,Sporočilo\n";
    if (ESP.getPsramSize() > 0) {
      csvContent.reserve(10000);
    }

    int count = 0;

    // Split file list and read each file (stream processing)
    int fileStart = 0;
    while (fileStart < logFiles.length()) {
      int commaPos = logFiles.indexOf(',', fileStart);
      String fileName = (commaPos == -1) ? logFiles.substring(fileStart) : logFiles.substring(fileStart, commaPos);

      if (fileName.length() > 0) {
        Serial.printf("WEB: Processing file %s\n", fileName.c_str());
        File f = SD_MMC.open(fileName.c_str(), FILE_READ);
        if (f) {
          while (f.available()) {
            String line = f.readStringUntil('\n');
            line.trim();

            if (line.length() > 0) {
              // Parse unix|unit|message
              int pipe1 = line.indexOf('|');
              int pipe2 = line.indexOf('|', pipe1 + 1);

              if (pipe1 > 0 && pipe2 > pipe1) {
                String unixStr = line.substring(0, pipe1);
                String unitStr = line.substring(pipe1 + 1, pipe2);
                String msgStr = line.substring(pipe2 + 1);

                uint32_t unix_time = unixStr.toInt();
                if (unix_time >= from_unix && unix_time <= to_unix) {
                  String localTime = myTZ.dateTime(unix_time, "H:i:s d.m.y");
                  csvContent += localTime + "," + String(unix_time) + "," + unitStr + "," + msgStr + "\n";
                  count++;
                }
              }
            }
          }
          f.close();
        } else {
          Serial.printf("WEB: Failed to open file: %s\n", fileName.c_str());
        }
      }

      fileStart = (commaPos == -1) ? logFiles.length() : commaPos + 1;
    }

    if (count == 0) {
      request->send(404, "text/plain", "Ni podatkov za izbrano obdobje");
      return;
    }

    logEvent("WEB: Export /logs for " + dateStr + " " + timeStr + ", " + String(count) + " entries");

    // Send CSV response
    AsyncWebServerResponse *response = request->beginResponse(200, "text/csv", csvContent);
    response->addHeader("Content-Disposition", "attachment; filename=logs.csv");
    request->send(response);
  });
}
