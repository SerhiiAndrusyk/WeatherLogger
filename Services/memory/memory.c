#include "memory.h"

static W25Q128_Handle flash;
extern SPI_HandleTypeDef hspi1;

void MemoryInit(){
	W25Q128_Init(&flash, &hspi1, FLASH_CS_GPIO_Port, FLASH_CS_Pin);
}

void MemoryRead(WeatherRecord *data, uint32_t address){
	W25Q128_Read(&flash, address, (uint8_t *)data, sizeof(WeatherRecord));
}

void MemoryWrite(WeatherRecord *data, uint32_t address){
	W25Q128_Write(&flash, address, (const uint8_t *)data, sizeof(WeatherRecord));
}

