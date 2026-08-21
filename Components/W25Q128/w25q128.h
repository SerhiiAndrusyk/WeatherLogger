#ifndef W25Q128_H
#define W25Q128_H

#include <stdint.h>
#include "stm32g4xx_hal.h"

#define W25Q128_SIZE         16777216UL
#define W25Q128_PAGE_SIZE         256U
#define W25Q128_SECTOR_SIZE      4096U

typedef enum{
    W25Q128_OK = 0,
    W25Q128_ERROR,
    W25Q128_TIMEOUT,
    W25Q128_INVALID_ARGUMENT,
    W25Q128_INVALID_ID
} W25Q128_Status;

typedef struct{
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *csPort;
    uint16_t csPin;
} W25Q128_Handle;

W25Q128_Status W25Q128_Init(W25Q128_Handle *flash, SPI_HandleTypeDef *hspi, GPIO_TypeDef *csPort, uint16_t csPin);
W25Q128_Status W25Q128_Read(W25Q128_Handle *flash, uint32_t address, uint8_t *data, uint32_t size);
W25Q128_Status W25Q128_Write(W25Q128_Handle *flash, uint32_t address, const uint8_t *data, uint32_t size);
W25Q128_Status W25Q128_EraseSector(W25Q128_Handle *flash, uint32_t address);

#endif
