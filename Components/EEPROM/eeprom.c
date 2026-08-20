#include "eeprom.h"

#define EEPROM_I2C_TIMEOUT_MS       200U
#define EEPROM_WRITE_TIMEOUT_MS      10U
#define EEPROM_READY_POLL_TIMEOUT_MS  1U

static EEPROM_Status EEPROM_WaitUntilReady(EEPROM_Handle *eeprom);


EEPROM_Status EEPROM_Init(EEPROM_Handle *eeprom, I2C_HandleTypeDef *hi2c, uint16_t address){
	if ((eeprom == NULL) || (hi2c == NULL)){
		return EEPROM_INVALID_ARGUMENT;
	}

	eeprom->hi2c = hi2c;
	eeprom->address = address;

    HAL_StatusTypeDef halStatus = HAL_I2C_IsDeviceReady(hi2c, address, 3U, EEPROM_I2C_TIMEOUT_MS);

    if (halStatus == HAL_OK){
        return EEPROM_OK;
    }

    return EEPROM_ERROR;
}

static EEPROM_Status EEPROM_WaitUntilReady(EEPROM_Handle *eeprom){
	if ((eeprom == NULL) || (eeprom->hi2c == NULL)){
		return EEPROM_INVALID_ARGUMENT;
	}

	uint32_t startTick = HAL_GetTick();

	while (HAL_GetTick() - startTick < EEPROM_WRITE_TIMEOUT_MS){
		HAL_StatusTypeDef halStatus =
				HAL_I2C_IsDeviceReady(eeprom->hi2c, eeprom->address, 1U, EEPROM_READY_POLL_TIMEOUT_MS);

		if (halStatus == HAL_OK){
			return EEPROM_OK;
		}
	}
	return EEPROM_TIMEOUT;
}

EEPROM_Status EEPROM_Read(EEPROM_Handle *eeprom, uint16_t memoryAddress, uint8_t *data, uint16_t size){
	if ((eeprom == NULL) ||
		(eeprom->hi2c == NULL) ||
		((uint32_t)memoryAddress + (uint32_t)size > EEPROM_SIZE) ||
		(data == NULL) ||
		(size == 0U)){
		return EEPROM_INVALID_ARGUMENT;
	}


	HAL_StatusTypeDef halStatus =
			HAL_I2C_Mem_Read(
					eeprom->hi2c,
					eeprom->address,
					memoryAddress, I2C_MEMADD_SIZE_16BIT,
					data,
					size,
					EEPROM_I2C_TIMEOUT_MS);

	if (halStatus == HAL_OK){
		return EEPROM_OK;
	}
	if (halStatus == HAL_TIMEOUT){
		return EEPROM_TIMEOUT;
	}
	return EEPROM_ERROR;
}

EEPROM_Status EEPROM_Write(EEPROM_Handle *eeprom, uint16_t memoryAddress, const uint8_t *data, uint16_t size){
    if ((eeprom == NULL) ||
        (eeprom->hi2c == NULL) ||
        (data == NULL) ||
        (size == 0U) ||
        ((uint32_t)memoryAddress + (uint32_t)size > EEPROM_SIZE)){
        return EEPROM_INVALID_ARGUMENT;
    }

    uint16_t pageOffset = memoryAddress % EEPROM_PAGE_SIZE;
    uint16_t chunkSize = EEPROM_PAGE_SIZE - pageOffset;

    while (size > 0U){

         if (chunkSize > size){
             chunkSize = size;
         }

         HAL_StatusTypeDef halStatus = HAL_I2C_Mem_Write(
             eeprom->hi2c,
             eeprom->address,
             memoryAddress,
             I2C_MEMADD_SIZE_16BIT,
             (uint8_t *)data,
             chunkSize,
             EEPROM_I2C_TIMEOUT_MS
         );

         if (halStatus == HAL_TIMEOUT){
             return EEPROM_TIMEOUT;
         }

         if (halStatus != HAL_OK){
             return EEPROM_ERROR;
         }

         EEPROM_Status status = EEPROM_WaitUntilReady(eeprom);

         if (status != EEPROM_OK){
             return status;
         }

         memoryAddress += chunkSize;
         data += chunkSize;
         size -= chunkSize;
         chunkSize = EEPROM_PAGE_SIZE;
     }

    return EEPROM_OK;
}
