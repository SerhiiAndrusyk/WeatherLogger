#include "memory.h"

static W25Q128_Handle flash;
extern SPI_HandleTypeDef hspi1;

void MemoryInit(){
	W25Q128_Init(&flash, &hspi1, FLASH_CS_GPIO_Port, FLASH_CS_Pin);
}

void MemoryRead(Sensor_Data *data, uint32_t address){
	W25Q128_Read(&flash, address, data, sizeof(Sensor_Data));
}

void MemoryWrite(Sensor_Data *data, uint32_t address){
	W25Q128_Write(&flash, address, data, sizeof(Sensor_Data));
}
