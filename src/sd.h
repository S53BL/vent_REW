// sd.h - SD card module header
#ifndef SD_H
#define SD_H

#include "config.h"
#include <SD_MMC.h>

// Function declarations
bool initSD();
void saveHistorySens();
void saveFanHistory();
void flushLogs();
String readFile(const char* path);
String listFiles(const char* pattern, uint32_t from_date = 0, uint32_t to_date = UINT32_MAX);
String listLogFiles(uint32_t from_date = 0, uint32_t to_date = UINT32_MAX);
uint32_t parseDateFromName(String name);

#endif // SD_H
