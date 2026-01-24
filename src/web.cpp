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

  server.on("/help", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", HTML_HELP);
  });

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
      while (start < sensFiles.length()) {
        int commaPos = sensFiles.indexOf(',', start);
        String fileName = (commaPos == -1) ? sensFiles.substring(start) : sensFiles.substring(start, commaPos);
        if (fileName.length() > 0 && SD_MMC.remove(fileName.c_str())) {
          deletedCount++;
        }
        start = (commaPos == -1) ? sensFiles.length() : commaPos + 1;
      }

      // Delete fan_history files
      String fanFiles = listFiles("fan_history_", 0, deleteBefore - 1);
      start = 0;
      while (start < fanFiles.length()) {
        int commaPos = fanFiles.indexOf(',', start);
        String fileName = (commaPos == -1) ? fanFiles.substring(start) : fanFiles.substring(start, commaPos);
        if (fileName.length() > 0 && SD_MMC.remove(fileName.c_str())) {
          deletedCount++;
        }
        start = (commaPos == -1) ? fanFiles.length() : commaPos + 1;
      }

      // Delete logs files (keep only last 7 days)
      String logFiles = listFiles("logs_", 0, deleteBefore - 1);
      start = 0;
      while (start < logFiles.length()) {
        int commaPos = logFiles.indexOf(',', start);
        String fileName = (commaPos == -1) ? logFiles.substring(start) : logFiles.substring(start, commaPos);
        if (fileName.length() > 0 && SD_MMC.remove(fileName.c_str())) {
          deletedCount++;
        }
        start = (commaPos == -1) ? logFiles.length() : commaPos + 1;
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

  server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request){
    if (sensorData.errorFlags[0] & ERR_SD) {
      request->send(503, "text/html", "<h1>SD ni na voljo</h1><a href='/'>Nazaj</a>");
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

    // Parse log entries
    struct LogEntry {
      uint32_t unix_time;
      String unit;
      String message;
    };

    std::vector<LogEntry> entries;

    // Split file list and read each file
    int fileStart = 0;
    while (fileStart < logFiles.length()) {
      int commaPos = logFiles.indexOf(',', fileStart);
      String fileName = (commaPos == -1) ? logFiles.substring(fileStart) : logFiles.substring(fileStart, commaPos);

      if (fileName.length() > 0) {
        String fileContent = readFile(fileName.c_str());
        if (fileContent.length() > 0) {
          // Parse lines
          int lineStart = 0;
          while (lineStart < fileContent.length()) {
            int lineEnd = fileContent.indexOf('\n', lineStart);
            if (lineEnd == -1) lineEnd = fileContent.length();

            String line = fileContent.substring(lineStart, lineEnd);
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
                  LogEntry entry;
                  entry.unix_time = unix_time;
                  entry.unit = unitStr;
                  entry.message = msgStr;
                  entries.push_back(entry);
                }
              }
            }

            lineStart = lineEnd + 1;
          }
        }
      }

      fileStart = (commaPos == -1) ? logFiles.length() : commaPos + 1;
    }

    // Sort entries by unix_time descending
    for (size_t i = 0; i < entries.size(); i++) {
      for (size_t j = i + 1; j < entries.size(); j++) {
        if (entries[j].unix_time > entries[i].unix_time) {
          LogEntry temp = entries[i];
          entries[i] = entries[j];
          entries[j] = temp;
        }
      }
    }

    // Build HTML table
    String tableHtml = "<div class=\"scrollable\"><table><thead><tr><th>Čas (lokalni)</th><th>Enota</th><th>Sporočilo</th></tr></thead><tbody>";

    int maxEntries = entries.size() > MAX_ROWS ? MAX_ROWS : entries.size();

    for (int i = 0; i < maxEntries; i++) {
      LogEntry entry = entries[i];

      // Determine row class based on message content
      String rowClass = "row-c"; // default green
      if (entry.message.indexOf("ERR") >= 0 || entry.message.indexOf("failed") >= 0) {
        rowClass = "row-r"; // red for errors
      }

      String localTime = myTZ.dateTime(entry.unix_time, "H:i:s d.m.y");
      tableHtml += "<tr class=\"" + rowClass + "\"><td>" + localTime + "</td><td>" + entry.unit + "</td><td>" + entry.message + "</td></tr>";
    }

    tableHtml += "</tbody></table></div>";

    if (entries.size() == 0) {
      tableHtml = "<p>Ni podatkov za izbrano obdobje.</p>";
    } else if (entries.size() > MAX_ROWS) {
      tableHtml += "<div class=\"warning\">Prikaz omejen na " + String(MAX_ROWS) + " vrstic; za polne podatke uporabi izvoz.</div>";
    }

    logEvent("WEB: Request /logs for " + dateStr + " " + timeStr + ", found " + String(entries.size()) + " entries");

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

    // List files
    const char* pattern = (typeStr == "sens") ? "history_sens_" : "fan_history_";
    String files = listFiles(pattern, fromDate, toDate);

    if (files.length() == 0) {
      request->send(404, "text/html", "<h1>Ni podatkov za izbrano obdobje</h1><a href='/'>Nazaj</a>");
      return;
    }

    // Collect all rows
    std::vector<std::vector<String>> allRows;

    // Parse files
    int fileStart = 0;
    while (fileStart < files.length()) {
      int commaPos = files.indexOf(',', fileStart);
      String fileName = (commaPos == -1) ? files.substring(fileStart) : files.substring(fileStart, commaPos);

      if (fileName.length() > 0) {
        String fileContent = readFile(fileName.c_str());
        if (fileContent.length() > 0) {
          // Parse CSV lines (skip header)
          int lineStart = fileContent.indexOf('\n') + 1; // Skip header
          while (lineStart < fileContent.length()) {
            int lineEnd = fileContent.indexOf('\n', lineStart);
            if (lineEnd == -1) lineEnd = fileContent.length();

            String line = fileContent.substring(lineStart, lineEnd);
            line.trim();

            if (line.length() > 0) {
              std::vector<String> row;
              int colStart = 0;
              while (colStart < line.length()) {
                int commaPos = line.indexOf(',', colStart);
                String col = (commaPos == -1) ? line.substring(colStart) : line.substring(colStart, commaPos);
                row.push_back(col);
                colStart = (commaPos == -1) ? line.length() : commaPos + 1;
              }
              if (row.size() >= (typeStr == "sens" ? 15 : 5)) { // Ensure we have all columns
                allRows.push_back(row);
              }
            }

            lineStart = lineEnd + 1;
          }
        }
      }

      fileStart = (commaPos == -1) ? files.length() : commaPos + 1;
    }

    if (allRows.size() == 0) {
      request->send(404, "text/html", "<h1>Ni podatkov za izbrano obdobje</h1><a href='/'>Nazaj</a>");
      return;
    }

    // Sort by time (column 0) ascending
    for (size_t i = 0; i < allRows.size(); i++) {
      for (size_t j = i + 1; j < allRows.size(); j++) {
        if (allRows[j][0] < allRows[i][0]) {
          std::vector<String> temp = allRows[i];
          allRows[i] = allRows[j];
          allRows[j] = temp;
        }
      }
    }

    // Build HTML table
    String tableHtml;
    if (typeStr == "sens") {
      tableHtml = "<table><thead><tr><th>Čas</th><th>Ext Temp</th><th>Ext Hum</th><th>Ext Pres</th><th>Ext Lux</th><th>DS Temp</th><th>DS Hum</th><th>DS CO2</th><th>Kop Temp</th><th>Kop Hum</th><th>Kop Pres</th><th>Ut Temp</th><th>Ut Hum</th><th>Wc Pres</th><th>Rezerva</th></tr></thead><tbody>";
    } else {
      tableHtml = "<table><thead><tr><th>Čas</th><th>Kop Fan</th><th>Ut Fan</th><th>Wc Fan</th><th>Common Intake</th></tr></thead><tbody>";
    }

    int maxRows = allRows.size() > MAX_ROWS ? MAX_ROWS : allRows.size();

    for (int i = 0; i < maxRows; i++) {
      tableHtml += "<tr>";
      for (size_t j = 0; j < allRows[i].size(); j++) {
        tableHtml += "<td>" + allRows[i][j] + "</td>";
      }
      tableHtml += "</tr>";
    }

    tableHtml += "</tbody></table>";

    if (allRows.size() > MAX_ROWS) {
      tableHtml += "<div class=\"warning\">Prikaz omejen na " + String(MAX_ROWS) + " vrstic; za polne podatke uporabi izvoz.</div>";
    }

    logEvent("WEB: Request /history for " + fromStr + " to " + toStr + ", type " + typeStr + ", found " + String(allRows.size()) + " rows");

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

    // Collect all rows
    std::vector<std::vector<String>> allRows;

    // Parse files
    int fileStart = 0;
    while (fileStart < files.length()) {
      int commaPos = files.indexOf(',', fileStart);
      String fileName = (commaPos == -1) ? files.substring(fileStart) : files.substring(fileStart, commaPos);

      if (fileName.length() > 0) {
        String fileContent = readFile(fileName.c_str());
        if (fileContent.length() > 0) {
          // Parse CSV lines (skip header)
          int lineStart = fileContent.indexOf('\n') + 1; // Skip header
          while (lineStart < fileContent.length()) {
            int lineEnd = fileContent.indexOf('\n', lineStart);
            if (lineEnd == -1) lineEnd = fileContent.length();

            String line = fileContent.substring(lineStart, lineEnd);
            line.trim();

            if (line.length() > 0) {
              std::vector<String> row;
              int colStart = 0;
              while (colStart < line.length()) {
                int commaPos = line.indexOf(',', colStart);
                String col = (commaPos == -1) ? line.substring(colStart) : line.substring(colStart, commaPos);
                row.push_back(col);
                colStart = (commaPos == -1) ? line.length() : commaPos + 1;
              }
              if (row.size() >= (typeStr == "sens" ? 15 : 5)) { // Ensure we have all columns
                allRows.push_back(row);
              }
            }

            lineStart = lineEnd + 1;
          }
        }
      }

      fileStart = (commaPos == -1) ? files.length() : commaPos + 1;
    }

    if (allRows.size() == 0) {
      request->send(404, "text/plain", "Ni podatkov za izbrano obdobje");
      return;
    }

    // Sort by time (column 0) ascending
    for (size_t i = 0; i < allRows.size(); i++) {
      for (size_t j = i + 1; j < allRows.size(); j++) {
        if (allRows[j][0] < allRows[i][0]) {
          std::vector<String> temp = allRows[i];
          allRows[i] = allRows[j];
          allRows[j] = temp;
        }
      }
    }

    // Build CSV content
    String csvContent;
    if (typeStr == "sens") {
      csvContent = "Čas zapisa,Temperatura zunaj,Vlaga zunaj,Tlak zunaj,VOC zunaj,Svetloba zunaj,Temperatura DS,Vlaga DS,CO2 DS,Temperatura UT,Vlaga UT,Temperatura KOP,Vlaga KOP,Tlak WC,Vremenski code\n";
    } else {
      csvContent = "Čas zapisa,WC stanje,UT stanje,KOP stanje,DS stanje\n";
    }

    for (size_t i = 0; i < allRows.size(); i++) {
      for (size_t j = 0; j < allRows[i].size(); j++) {
        if (j > 0) csvContent += ",";
        csvContent += allRows[i][j];
      }
      csvContent += "\n";
    }

    logEvent("WEB: Download /history/" + typeStr + " from " + fromStr + " to " + toStr + ", " + String(allRows.size()) + " rows");

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

    // Parse log entries
    struct LogEntry {
      uint32_t unix_time;
      String unit;
      String message;
    };

    std::vector<LogEntry> entries;

    // Split file list and read each file
    int fileStart = 0;
    while (fileStart < logFiles.length()) {
      int commaPos = logFiles.indexOf(',', fileStart);
      String fileName = (commaPos == -1) ? logFiles.substring(fileStart) : logFiles.substring(fileStart, commaPos);

      if (fileName.length() > 0) {
        String fileContent = readFile(fileName.c_str());
        if (fileContent.length() > 0) {
          // Parse lines
          int lineStart = 0;
          while (lineStart < fileContent.length()) {
            int lineEnd = fileContent.indexOf('\n', lineStart);
            if (lineEnd == -1) lineEnd = fileContent.length();

            String line = fileContent.substring(lineStart, lineEnd);
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
                  LogEntry entry;
                  entry.unix_time = unix_time;
                  entry.unit = unitStr;
                  entry.message = msgStr;
                  entries.push_back(entry);
                }
              }
            }

            lineStart = lineEnd + 1;
          }
        }
      }

      fileStart = (commaPos == -1) ? logFiles.length() : commaPos + 1;
    }

    if (entries.size() == 0) {
      request->send(404, "text/plain", "Ni podatkov za izbrano obdobje");
      return;
    }

    // Sort entries by unix_time descending
    for (size_t i = 0; i < entries.size(); i++) {
      for (size_t j = i + 1; j < entries.size(); j++) {
        if (entries[j].unix_time > entries[i].unix_time) {
          LogEntry temp = entries[i];
          entries[i] = entries[j];
          entries[j] = temp;
        }
      }
    }

    // Build CSV content
    String csvContent = "Čas (lokalni),Unix čas,Enota,Sporočilo\n";

    for (size_t i = 0; i < entries.size(); i++) {
      LogEntry entry = entries[i];
      String localTime = myTZ.dateTime(entry.unix_time, "H:i:s d.m.y");
      csvContent += localTime + "," + String(entry.unix_time) + "," + entry.unit + "," + entry.message + "\n";
    }

    logEvent("WEB: Export /logs for " + dateStr + " " + timeStr + ", " + String(entries.size()) + " entries");

    // Send CSV response
    AsyncWebServerResponse *response = request->beginResponse(200, "text/csv", csvContent);
    response->addHeader("Content-Disposition", "attachment; filename=logs.csv");
    request->send(response);
  });
}