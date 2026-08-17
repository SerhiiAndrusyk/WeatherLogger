#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include "stm32g4xx_hal.h"

#define SSD1306_WIDTH 128U
#define SSD1306_HEIGHT 64U
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH*SSD1306_HEIGHT/8U)

typedef enum{
	SSD1306_OK = 0,
	SSD1306_ERROR,
	SSD1306_TIMEOUT,
	SSD1306_INVALID_ARGUMENT
} SSD1306_Status;

typedef struct{
	I2C_HandleTypeDef *hi2c;
	uint16_t address;
} SSD1306_Handle;

SSD1306_Status SSD1306_Init(SSD1306_Handle *display, I2C_HandleTypeDef *hi2c, uint16_t address);
SSD1306_Status SSD1306_Clear(void);
SSD1306_Status SSD1306_UpdateScreen(SSD1306_Handle *display);
SSD1306_Status SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color);

#endif
