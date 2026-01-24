// logging.h - Logging system header
#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>

// Function declarations
void logEvent(String msg);
void flushBufferToSD();
void initLogging();
void cleanupOldLogs();

#endif // LOGGING_H