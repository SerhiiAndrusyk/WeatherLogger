#include "display.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;
extern uint32_t countDayRecords;
extern uint32_t countMonthRecords;
extern uint32_t countYearRecords;
static SSD1306_Handle display;

static void DISPLAY_WriteDate(const char *date){
	size_t length = strlen(date);
	size_t width = length == 0U ? 0U : length * (SSD1306_Font5x7.width + 1U) - 1U;
	int16_t x = width < SSD1306_WIDTH ? (int16_t)((SSD1306_WIDTH - width) / 2U) : 0;
	SSD1306_DrawString(x, 0, date, &SSD1306_Font5x7, 1);
}

void DISPLAY_Init(){
	SSD1306_Init(&display, &hi2c1, 0x3C << 1);
}

void DISPLAY_WriteStart(){
	SSD1306_Clear();
	SSD1306_DrawString(0, 0, "Single click to", &SSD1306_Font5x7, 1);
	SSD1306_DrawString(0, 8, "change page", &SSD1306_Font5x7, 1);
	SSD1306_DrawString(0, 16, "Double click to", &SSD1306_Font5x7, 1);
	SSD1306_DrawString(0, 24, "change mode", &SSD1306_Font5x7, 1);
	SSD1306_DrawString(0, 32, "mode 0: menu", &SSD1306_Font5x7, 1);
	SSD1306_DrawString(0, 40, "mode 1: data by day", &SSD1306_Font5x7, 1);
	SSD1306_DrawString(0, 48, "mode 2: data by month", &SSD1306_Font5x7, 1);
	SSD1306_DrawString(0, 56, "mode 3: data by year", &SSD1306_Font5x7, 1);
	SSD1306_UpdateScreen(&display);
}


void DISPLAY_WritePage(uint8_t mode, uint16_t page){
	WeatherRecord temporary;
	WeatherRecord first;
	char weather[LEN_OF_WEATHER];
	char date[20];
	switch (mode) {
		case 1:
			SSD1306_Clear();
			if ((page*RECORDS_BY_PAGE) >= countDayRecords){
				SSD1306_DrawString(43, 0, "No data", &SSD1306_Font5x7, 1);
			}
			for (uint8_t i = 0; i < RECORDS_BY_PAGE; i++){
				if ((page*RECORDS_BY_PAGE+i) >= countDayRecords){
					SSD1306_DrawString(0, 8*(i+2), "No data", &SSD1306_Font5x7, 1);
				}
				else {
					MemoryRead(&temporary, (DAYS_ADDRESS+(page*RECORDS_BY_PAGE+i)*sizeof(WeatherRecord)));
					snprintf(weather, sizeof(weather), "%dC, %dPa, %d%%",
							(int)temporary.data.temperature,
							(int)temporary.data.pressure,
							(int)temporary.data.humidity);
					SSD1306_DrawString(0, 8*(i+2), weather, &SSD1306_Font5x7, 1);
				}
			}
			if ((page*RECORDS_BY_PAGE) < countDayRecords){
				MemoryRead(&temporary, (DAYS_ADDRESS+(page*RECORDS_BY_PAGE)*sizeof(WeatherRecord)));
				snprintf(date, sizeof(date), "%d.%d.%d",
					(int)temporary.day,
					(int)temporary.month,
					(int)temporary.year);
				DISPLAY_WriteDate(date);
			}
			break;
		case 2:
			SSD1306_Clear();
			if ((page*RECORDS_BY_PAGE) >= countMonthRecords){
				SSD1306_DrawString(43, 0, "No data", &SSD1306_Font5x7, 1);
			}
			else{
				MemoryRead(&temporary, (MONTHS_ADDRESS+(page*RECORDS_BY_PAGE)*sizeof(WeatherRecord)));
				first = temporary;
			}
			for (uint8_t i = 0; i < RECORDS_BY_PAGE; i++){
				if ((page*RECORDS_BY_PAGE+i) >= countMonthRecords){
					SSD1306_DrawString(0, 8*(i+2), "No data", &SSD1306_Font5x7, 1);
				}
				else {
					MemoryRead(&temporary, (MONTHS_ADDRESS+(page*RECORDS_BY_PAGE+i)*sizeof(WeatherRecord)));
					snprintf(weather, sizeof(weather), "%dC, %dPa, %d%%",
							(int)temporary.data.temperature,
							(int)temporary.data.pressure,
							(int)temporary.data.humidity);
					SSD1306_DrawString(0, 8*(i+2), weather, &SSD1306_Font5x7, 1);
				}
			}
			if ((page*RECORDS_BY_PAGE) < countMonthRecords){
				snprintf(date, sizeof(date), "%d.%d.%d-%d.%d.%d",
					(int)first.day,
					(int)first.month,
					(int)(first.year % 100U),
					(int)temporary.day,
					(int)temporary.month,
					(int)(temporary.year % 100U));
				DISPLAY_WriteDate(date);
			}
			break;
		case 3:
			SSD1306_Clear();
			if ((page*RECORDS_BY_PAGE) >= countYearRecords){
				SSD1306_DrawString(43, 0, "No data", &SSD1306_Font5x7, 1);
			}
			else{
				MemoryRead(&temporary, (YEARS_ADDRESS+(page*RECORDS_BY_PAGE)*sizeof(WeatherRecord)));
				first = temporary;
			}
			for (uint8_t i = 0; i < RECORDS_BY_PAGE; i++){
				if ((page*RECORDS_BY_PAGE+i) >= countYearRecords){
					SSD1306_DrawString(0, 8*(i+2), "No data", &SSD1306_Font5x7, 1);
				}
				else {
					MemoryRead(&temporary, (YEARS_ADDRESS+(page*RECORDS_BY_PAGE+i)*sizeof(WeatherRecord)));
					snprintf(weather, sizeof(weather), "%dC, %dPa, %d%%",
							(int)temporary.data.temperature,
							(int)temporary.data.pressure,
							(int)temporary.data.humidity);
					SSD1306_DrawString(0, 8*(i+2), weather, &SSD1306_Font5x7, 1);
				}
			}
			if ((page*RECORDS_BY_PAGE) < countYearRecords){
				snprintf(date, sizeof(date), "%d.%d-%d.%d",
					(int)first.month,
					(int)first.year,
					(int)temporary.month,
					(int)temporary.year);
				DISPLAY_WriteDate(date);
			}
			break;
	}
	SSD1306_UpdateScreen(&display);
}

void DISPLAY_On(){
	SSD1306_On(&display);
}
void DISPLAY_Off(){
	SSD1306_Off(&display);
}
