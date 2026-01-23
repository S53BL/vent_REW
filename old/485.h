// 485.h
#ifndef RS485_H
#define RS485_H
#include <Arduino.h>
#include "config.h"
#include "ezTime.h"
#include "display.h"
extern volatile bool dnTriggered;
extern bool waitingResponse;
extern unsigned long lastRequestSent;
extern int retryRequestCount;
extern uint8_t historyBuffer[15000]; // Buffer za HISTORY_RESPONSE (do 15 KB)
extern size_t historyBufferSize;
extern bool waitingForConfirmAction; // Za obdelavo CONFIRM_ACTION
extern unsigned long lastParamUpdateSent; // Za dostop do timeout spremenljivke
extern bool waitingForManual; // Za MANUAL_CONTROL
extern unsigned long lastManualSent; // Za MANUAL_CONTROL timeout
extern unsigned long lastValidTime, lastTimeUpdate; // Dodano za interni čas
void setupRS485();
void receiveRS485();
void sendViaRS485(uint8_t* data, size_t len);
void handleDNInterrupt();
void sendSensorData();
void sendManualControl(uint8_t roomID, uint8_t command);
void sendGraphRequest(GraphType type);
#endif // RS485_H