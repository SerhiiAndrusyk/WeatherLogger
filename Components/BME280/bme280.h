#ifndef BME280_H
#define BME280_H

#include <stdint.h>
#include "stm32g4xx_hal.h"

typedef enum{
	BME280_OK = 0,
	BME280_ERROR,
	BME280_TIMEOUT,
	BME280_INVALID_ARGUMENT
} BME280_Status;

typedef struct{
    I2C_HandleTypeDef *hi2c;
    uint16_t address;

    uint16_t digT1;
    int16_t digT2;
    int16_t digT3;

    uint16_t digP1;
    int16_t digP2;
    int16_t digP3;
    int16_t digP4;
    int16_t digP5;
    int16_t digP6;
    int16_t digP7;
    int16_t digP8;
    int16_t digP9;

    uint8_t digH1;
    int16_t digH2;
    uint8_t digH3;
    int16_t digH4;
    int16_t digH5;
    int8_t digH6;

    int32_t tFine;
} BME280_Handle;

typedef struct{
    float temperature;
    float pressure;
    float humidity;
} BME280_Data;

BME280_Status BME280_Init(BME280_Handle *sensor, I2C_HandleTypeDef *hi2c, uint16_t address);
BME280_Status BME280_Read(BME280_Handle *sensor, BME280_Data *data);

#endif
