#include "ssd1306.h"
#include <string.h>

#define SSD1306_CONTROL_COMMAND 0x00U
#define SSD1306_CONTROL_DATA 0x40U

#define SSD1306_I2C_TIMEOUT_MS 1000U
#define SSD1306_MAX_COMMAND_BYTES 32U
#define SSD1306_I2C_CHUNK_SIZE 32U

static uint8_t framebuffer[SSD1306_BUFFER_SIZE];

static SSD1306_Status SSD1306_WriteCommand(SSD1306_Handle *display, const uint8_t *commands, uint8_t size);

static SSD1306_Status SSD1306_WriteData(SSD1306_Handle *display, const uint8_t *data, uint16_t size);

static SSD1306_Status SSD1306_WriteCommand(SSD1306_Handle *display, const uint8_t *commands, uint8_t size){
    if ((display == NULL) ||
        (display->hi2c == NULL) ||
        (commands == NULL) ||
        (size == 0U) ||
        (size > SSD1306_MAX_COMMAND_BYTES))
    {
        return SSD1306_INVALID_ARGUMENT;
    }

    uint8_t packet[SSD1306_MAX_COMMAND_BYTES + 1U];

    packet[0] = SSD1306_CONTROL_COMMAND;
    memcpy(&packet[1], commands, size);

    HAL_StatusTypeDef halStatus =
        HAL_I2C_Master_Transmit(
            display->hi2c,
            display->address,
            packet,
            size + 1U,
            SSD1306_I2C_TIMEOUT_MS
        );

    if (halStatus == HAL_OK) {
        return SSD1306_OK;
    }

    uint32_t error =
        HAL_I2C_GetError(display->hi2c);

    if ((error & HAL_I2C_ERROR_TIMEOUT) != 0U) {
        return SSD1306_TIMEOUT;
    }

    return SSD1306_ERROR;
}

static SSD1306_Status SSD1306_WriteData(SSD1306_Handle *display, const uint8_t *data, uint16_t size){
    if ((display == NULL) ||
        (display->hi2c == NULL) ||
        (data == NULL) ||
        (size == 0U))
    {
        return SSD1306_INVALID_ARGUMENT;
    }

    uint8_t packet[SSD1306_I2C_CHUNK_SIZE + 1U];

    while (size > 0U) {
        uint16_t chunkSize = size;

        if (chunkSize > SSD1306_I2C_CHUNK_SIZE) {
            chunkSize = SSD1306_I2C_CHUNK_SIZE;
        }

        packet[0] = SSD1306_CONTROL_DATA;
        memcpy(&packet[1], data, chunkSize);

        HAL_StatusTypeDef halStatus =
            HAL_I2C_Master_Transmit(
                display->hi2c,
                display->address,
                packet,
                chunkSize + 1U,
                SSD1306_I2C_TIMEOUT_MS
            );

        if (halStatus != HAL_OK) {
            uint32_t error =
                HAL_I2C_GetError(display->hi2c);

            if ((error & HAL_I2C_ERROR_TIMEOUT) != 0U) {
                return SSD1306_TIMEOUT;
            }

            return SSD1306_ERROR;
        }

        data += chunkSize;
        size -= chunkSize;
    }

    return SSD1306_OK;
}

SSD1306_Status SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color){
	if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT){
		return SSD1306_INVALID_ARGUMENT;
	}
	uint16_t index = (uint16_t)x+((uint16_t)y/8U)*SSD1306_WIDTH;
	uint8_t bit = 1U << (uint16_t)y%8U;
	if (color){
		framebuffer[index] |= (bit);
	}
	else {
		framebuffer[index] &= ~(bit);
	}
	return SSD1306_OK;
}

SSD1306_Status SSD1306_Init(SSD1306_Handle *display, I2C_HandleTypeDef *hi2c, uint16_t address){
	if ((hi2c == NULL) || (display == NULL)){
		return SSD1306_INVALID_ARGUMENT;
	}

	display->hi2c = hi2c;
	display->address = address;

    const uint8_t commands[] = {
        0xAE,       // Display OFF

        0xD5, 0x80, // Display clock
        0xA8, 0x3F, // Multiplex ratio: 64 rows
        0xD3, 0x00, // Display offset
        0x40,       // Start line = 0

        0x8D, 0x14, // Enable charge pump
        0x20, 0x00, // Horizontal addressing mode

        0xA1,       // Segment remap
        0xC8,       // COM scan direction
        0xDA, 0x12, // COM pins configuration

        0x81, 0x7F, // Contrast
        0xD9, 0xF1, // Pre-charge period
        0xDB, 0x40, // VCOMH deselect level

        0xA4,       // Display follows RAM
        0xA6,       // Normal display
        0x2E,       // Disable scrolling
        0xAF        // Display ON
    };

    SSD1306_Status status;

    status = SSD1306_WriteCommand(display, commands, sizeof(commands));

    if (status != SSD1306_OK) {
        return status;
    }

    SSD1306_Clear();
    return SSD1306_UpdateScreen(display);
}

SSD1306_Status SSD1306_UpdateScreen(SSD1306_Handle *display){
	if ((display == NULL) || (display->hi2c == NULL)){
		return SSD1306_INVALID_ARGUMENT;
	}

    const uint8_t commands[] = {
        0x21, 0x00, SSD1306_WIDTH - 1U,
        0x22, 0x00, (SSD1306_HEIGHT / 8U) - 1U
    };
    SSD1306_Status status = SSD1306_WriteCommand(display, commands, sizeof(commands));

    if (status != SSD1306_OK) {
        return status;
    }
    return SSD1306_WriteData(display, framebuffer, SSD1306_BUFFER_SIZE);
}

SSD1306_Status SSD1306_Clear(void){
	memset(framebuffer, 0, sizeof(framebuffer));
	return SSD1306_OK;
}

