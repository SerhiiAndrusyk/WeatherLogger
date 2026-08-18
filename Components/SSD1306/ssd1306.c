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

static void SSD1306_DrawPixelClipped(int16_t x, int16_t y, uint8_t color);

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

SSD1306_Status SSD1306_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color){
	if ((x0 >= SSD1306_WIDTH) || (x1 >= SSD1306_WIDTH) ||
		(y0 >= SSD1306_HEIGHT) || (y1 >= SSD1306_HEIGHT)){
		return SSD1306_INVALID_ARGUMENT;
	}

	int16_t dx = (x1 > x0) ? (int16_t)(x1 - x0) : (int16_t)(x0 - x1);
	int16_t dy = (y1 > y0) ? (int16_t)(y1 - y0) : (int16_t)(y0 - y1);

	int16_t sx = (x0 < x1) ? 1 : -1;
	int16_t sy = (y0 < y1) ? 1 : -1;
	int16_t error = dx - dy;

	while (1){
		SSD1306_Status status = SSD1306_DrawPixel(x0, y0, color);
		if (status != SSD1306_OK){
			return status;
		}

		if ((x0 == x1) && (y0 == y1)){
			break;
		}

		int16_t error2 = 2 * error;

		if (error2 > -dy){
			error -= dy;
			x0 = (uint8_t)((int16_t)x0 + sx);
		}

		if (error2 < dx){
			error += dx;
			y0 = (uint8_t)((int16_t)y0 + sy);
		}
	}

	return SSD1306_OK;
}

static void SSD1306_DrawPixelClipped(int16_t x, int16_t y, uint8_t color){
	if ((x < 0) || (x >= SSD1306_WIDTH) ||
		(y < 0) || (y >= SSD1306_HEIGHT)){
		return;
	}

	SSD1306_DrawPixel((uint8_t)x, (uint8_t)y, color);
}

SSD1306_Status SSD1306_DrawRectangle(int16_t x, int16_t y, uint8_t width, uint8_t height, uint8_t color){
	if ((height == 0U) || (width == 0U)){
		return SSD1306_INVALID_ARGUMENT;
	}

	for (uint8_t i = 0; i < width; i++){
		SSD1306_DrawPixelClipped(x+i, y, color);
	}
	for (uint8_t i = 0; i < width; i++){
		SSD1306_DrawPixelClipped(x+i, y+height-1, color);
	}
	for (uint8_t i = 0; i < height; i++){
		SSD1306_DrawPixelClipped(x, y+i, color);
	}
	for (uint8_t i = 0; i < height; i++){
		SSD1306_DrawPixelClipped(x+width-1, y+i, color);
	}

	return SSD1306_OK;
}

SSD1306_Status SSD1306_FillRectangle(int16_t x, int16_t y, uint8_t width, uint8_t height, uint8_t color){
	if ((height == 0U) || (width == 0U)){
		return SSD1306_INVALID_ARGUMENT;
	}

	for (uint8_t i = 0; i < height; i++){
		for (uint8_t j = 0; j < width; j++){
			SSD1306_DrawPixelClipped(x+j, y+i, color);
		}
	}

	return SSD1306_OK;
}

SSD1306_Status SSD1306_DrawCircle(int16_t centerX, int16_t centerY, uint8_t radius, uint8_t color){
	if (radius == 0U){
	    SSD1306_DrawPixelClipped(centerX, centerY, color);
	    return SSD1306_OK;
	}

    int16_t x = 0;
    int16_t y = radius;
    int16_t decision = 1 - radius;

    while (x <= y){
        SSD1306_DrawPixelClipped(centerX + x, centerY + y, color);
        SSD1306_DrawPixelClipped(centerX - x, centerY + y, color);
        SSD1306_DrawPixelClipped(centerX + x, centerY - y, color);
        SSD1306_DrawPixelClipped(centerX - x, centerY - y, color);

        SSD1306_DrawPixelClipped(centerX + y, centerY + x, color);
        SSD1306_DrawPixelClipped(centerX - y, centerY + x, color);
        SSD1306_DrawPixelClipped(centerX + y, centerY - x, color);
        SSD1306_DrawPixelClipped(centerX - y, centerY - x, color);

        x++;

        if (decision < 0){
            decision += 2 * x + 1;
        }
        else{
            y--;
            decision += 2 * (x - y) + 1;
        }
    }

    return SSD1306_OK;
}

SSD1306_Status SSD1306_FillCircle(int16_t centerX, int16_t centerY, uint8_t radius, uint8_t color){
    int16_t x = 0;
    int16_t y = radius;
    int16_t decision = 1 - radius;

    while (x <= y){
        for (int16_t i = centerX - x; i <= centerX + x; i++){
            SSD1306_DrawPixelClipped(i, centerY + y, color);
            SSD1306_DrawPixelClipped(i, centerY - y, color);
        }

        for (int16_t i = centerX - y; i <= centerX + y; i++){
            SSD1306_DrawPixelClipped(i, centerY + x, color);
            SSD1306_DrawPixelClipped(i, centerY - x, color);
        }

        x++;

        if (decision < 0){
            decision += 2 * x + 1;
        }
        else{
            y--;
            decision += 2 * (x - y) + 1;
        }
    }

    return SSD1306_OK;
}

SSD1306_Status SSD1306_DrawEllipse(int16_t centerX, int16_t centerY, uint8_t radiusX, uint8_t radiusY, uint8_t color){
    if ((radiusX == 0U) || (radiusY == 0U)){
        return SSD1306_INVALID_ARGUMENT;
    }

    int32_t x = 0;
    int32_t y = radiusY;

    int32_t radiusX2 = (int32_t)radiusX * radiusX;
    int32_t radiusY2 = (int32_t)radiusY * radiusY;

    int32_t dx = 0;
    int32_t dy = 2 * radiusX2 * y;

    int32_t decision = radiusY2 - radiusX2 * radiusY + radiusX2 / 4;

    while (dx < dy){
        SSD1306_DrawPixelClipped(centerX + x, centerY + y, color);
        SSD1306_DrawPixelClipped(centerX - x, centerY + y, color);
        SSD1306_DrawPixelClipped(centerX + x, centerY - y, color);
        SSD1306_DrawPixelClipped(centerX - x, centerY - y, color);

        x++;
        dx += 2 * radiusY2;

        if (decision < 0){
            decision += radiusY2 + dx;
        }
        else{
            y--;
            dy -= 2 * radiusX2;
            decision += radiusY2 + dx - dy;
        }
    }

    decision =
        radiusY2 * (x * x + x) +
        radiusY2 / 4 +
        radiusX2 * (y - 1) * (y - 1) -
        radiusX2 * radiusY2;

    while (y >= 0){
        SSD1306_DrawPixelClipped(centerX + x, centerY + y, color);
        SSD1306_DrawPixelClipped(centerX - x, centerY + y, color);
        SSD1306_DrawPixelClipped(centerX + x, centerY - y, color);
        SSD1306_DrawPixelClipped(centerX - x, centerY - y, color);

        y--;
        dy -= 2 * radiusX2;

        if (decision > 0){
            decision += radiusX2 - dy;
        }
        else{
            x++;
            dx += 2 * radiusY2;
            decision += radiusX2 - dy + dx;
        }
    }

    return SSD1306_OK;
}

SSD1306_Status SSD1306_FillEllipse(int16_t centerX, int16_t centerY, uint8_t radiusX, uint8_t radiusY, uint8_t color){
    if ((radiusX == 0U) || (radiusY == 0U)){
        return SSD1306_INVALID_ARGUMENT;
    }

    int32_t x = 0;
    int32_t y = radiusY;

    int32_t radiusX2 = (int32_t)radiusX * radiusX;
    int32_t radiusY2 = (int32_t)radiusY * radiusY;

    int32_t dx = 0;
    int32_t dy = 2 * radiusX2 * y;

    int32_t decision = radiusY2 - radiusX2 * radiusY + radiusX2 / 4;

    while (dx < dy){
    	for (int16_t i = centerX - x; i <= centerX + x; i++){
    		SSD1306_DrawPixelClipped(i, centerY + y, color);
    		SSD1306_DrawPixelClipped(i, centerY - y, color);
    	}

        x++;
        dx += 2 * radiusY2;

        if (decision < 0){
            decision += radiusY2 + dx;
        }
        else{
            y--;
            dy -= 2 * radiusX2;
            decision += radiusY2 + dx - dy;
        }
    }

    decision =
        radiusY2 * (x * x + x) +
        radiusY2 / 4 +
        radiusX2 * (y - 1) * (y - 1) -
        radiusX2 * radiusY2;

    while (y >= 0){
    	for (int16_t i = centerX - x; i <= centerX + x; i++){
    		SSD1306_DrawPixelClipped(i, centerY + y, color);
    		SSD1306_DrawPixelClipped(i, centerY - y, color);
    	}

        y--;
        dy -= 2 * radiusX2;

        if (decision > 0){
            decision += radiusX2 - dy;
        }
        else{
            x++;
            dx += 2 * radiusY2;
            decision += radiusX2 - dy + dx;
        }
    }

    return SSD1306_OK;
}
