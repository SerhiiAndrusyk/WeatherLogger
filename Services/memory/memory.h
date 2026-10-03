#ifndef MEMORY
#define MEMORY

#define DAYS_ADDRESS 0x00000000
#define MONTHS_ADDRESS 0x00F57000
#define YEARS_ADDRESS 0x00FFB000

#define WEATHER 0U
#define DATE 1U

#define LEN_OF_WEATHER 20U
#define LEN_OF_DATE 20U

#include "w25q128.h"
#include "sensor.h"
#include "main.h"
#include <stdio.h>

typedef struct{
	Sensor_Data data;
	uint16_t year;
	uint8_t month;
	uint8_t day;
}WeatherRecord;

void MemoryInit();
void MemoryRead(WeatherRecord *data, uint32_t address);
void MemoryWrite(WeatherRecord *data, uint32_t address);

#endif
