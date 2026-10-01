#include "sensor.h"

extern I2C_HandleTypeDef hi2c1;
static BME280_Handle sensor;

void SensorInit(){
	BME280_Init(sensor, &hi2c1, 0x76 << 1);
}
void SensorRead(Sensor_Data *data){
	BME280_Read(sensor, data);
}
