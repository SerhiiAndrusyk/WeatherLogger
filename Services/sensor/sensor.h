#ifndef SENSOR
#define SENSOR

#include "bme280.h"

typedef BME280_Data Sensor_Data;

void SensorInit();
void SensorRead(Sensor_Data *data);

#endif
