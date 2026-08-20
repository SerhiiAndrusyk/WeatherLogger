#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include "stm32g4xx_hal.h"

#define EEPROM_SIZE       4096U
#define EEPROM_PAGE_SIZE  32U

typedef enum {
	EEPROM_OK = 0,
	EEPROM_ERROR,
	EEPROM_TIMEOUT,
	EEPROM_INVALID_ARGUMENT
} EEPROM_Status;

typedef struct{
	I2C_HandleTypeDef *hi2c;
	uint16_t address;
} EEPROM_Handle;


EEPROM_Status EEPROM_Init(EEPROM_Handle *eeprom, I2C_HandleTypeDef *hi2c, uint16_t address);
EEPROM_Status EEPROM_Read(EEPROM_Handle *eeprom, uint16_t memoryAddress, uint8_t *data, uint16_t size);
EEPROM_Status EEPROM_Write(EEPROM_Handle *eeprom, uint16_t memoryAddress, const uint8_t *data, uint16_t size);

#endif
