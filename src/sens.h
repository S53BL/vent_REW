// sens.h - Sensor module header
#ifndef SENS_H
#define SENS_H

#include "config.h"

// Function declarations
bool initSensors();
void readSensors();
void resetSensors();
float getLuxLevel();

#endif // SENS_H
