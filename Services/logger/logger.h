#ifndef LOGGER
#define LOGGER

#include "sensor.h"
#include "memory.h"

typedef struct{
    float temperatureSum;
    float pressureSum;
    float humiditySum;
    uint16_t count;
} WeatherAccumulator;

void Logger_Process();

#endif
