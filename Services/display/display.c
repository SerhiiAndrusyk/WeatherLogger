#include "display.h"

extern I2C_HandleTypeDef hi2c1;
static SSD1306_Handle display;

static int16_t scrollX = 127;

void DISPLAY_Init(){
	SSD1306_Init(&display, &hi2c1, 0x3C << 1);
}

void DISPLAY_WriteStart(){
	scrollX = 127;
	SSD1306_Clear();
	SSD1306_DrawString(0, 20, "mode 0: menu", &SSD1306_Font5x7, 1);
	SSD1306_DrawString(0, 30, "mode 1: data by day", &SSD1306_Font5x7, 1);
	SSD1306_DrawString(0, 40, "mode 2: data by month", &SSD1306_Font5x7, 1);
	SSD1306_DrawString(0, 50, "mode 3: data by year", &SSD1306_Font5x7, 1);
	SSD1306_UpdateScreen(&display);
}

void DISPLAY_ScrollWrite(){
	SSD1306_FillRectangle(0, 0, 128, 20, 0);
	SSD1306_DrawString(scrollX, 0, "Single click to change mode", &SSD1306_Font5x7, 1);
	SSD1306_DrawString(scrollX, 10, "Double click to change page", &SSD1306_Font5x7, 1);
	SSD1306_UpdateScreen(&display);

	scrollX -= 2;

	if (scrollX < -162){
		scrollX = 127;
	}
}
