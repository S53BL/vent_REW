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

#endif // SD_H
