// logging.cpp - Logging system implementation
#include "logging.h"
#include "globals.h"
#include "sd.h"
#include <SD_MMC.h>

uint32_t lastFlush = 0;

void initLogging() {
    logBuffer = "";
    lastFlush = millis();
    loggingInitialized = true;
    cleanupOldLogs();
}

void logEvent(String content) {
    // Add timestamp internally
    String timestamp;
    if (timeSynced) {
        timestamp = String(myTZ.now());
    } else {
        timestamp = "M" + String(millis());
    }

    String logLine = timestamp + "|R|" + content + "\n";

    // Always to Serial
    Serial.print(logLine);

    // Add to RAM buffer if logging initialized
    if (loggingInitialized) {
        logBuffer += logLine;

        // Check if buffer is full or timeout exceeded
        if (logBuffer.length() > LOG_BUFFER_MAX ||
            (millis() - lastFlush > 300000)) {  // 5 min timeout
            flushBufferToSD();
        }
    }
}

void flushBufferToSD() {
    if (sensorData.errorFlags[0] & ERR_SD) {
        Serial.println("[LOG] SD ERR - skipping flush");
        return;
    }

    if (logBuffer.length() == 0) {
        return;  // Nothing to flush
    }

    // Create filename based on current date
    String currentDate = myTZ.dateTime("Ymd");
    String logFileName = "/logs_" + currentDate + ".txt";

    // Open file for append
    File logFile = SD_MMC.open(logFileName.c_str(), FILE_APPEND);
    if (!logFile) {
        Serial.println("[LOG] Failed to open log file: " + logFileName);
        return;
    }

    // Write buffer to file
    size_t bytesWritten = logFile.print(logBuffer);
    logFile.close();

    if (bytesWritten > 0) {
        Serial.printf("[LOG] Flushed %d bytes to %s\n", bytesWritten, logFileName.c_str());
        logBuffer = "";  // Clear buffer
        lastFlush = millis();
    } else {
        Serial.println("[LOG] Failed to write to log file");
    }
}

void cleanupOldLogs() {
    if (sensorData.errorFlags[0] & ERR_SD) {
        return;
    }

    File root = SD_MMC.open("/");
    if (!root) {
        Serial.println("[LOG] Failed to open root directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        String fileName = file.name();
        file.close();

        // Check if it's a log file
        if (fileName.startsWith("/logs_") && fileName.endsWith(".txt")) {
            // Extract date from filename (logs_YYYYMMDD.txt)
            String dateStr = fileName.substring(6, 14);  // Extract YYYYMMDD

            // Calculate age in days
            // For simplicity, just check if it's older than 7 days
            // In production, this should be more sophisticated
            uint32_t fileDate = dateStr.toInt();
            uint32_t currentDate = myTZ.dateTime("Ymd").toInt();

            if (currentDate - fileDate > 7) {
                // Delete old log file
                if (SD_MMC.remove(fileName.c_str())) {
                    Serial.println("[LOG] Deleted old log: " + fileName);
                } else {
                    Serial.println("[LOG] Failed to delete: " + fileName);
                }
            }
        }

        file = root.openNextFile();
    }
    root.close();
}