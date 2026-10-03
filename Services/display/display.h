#ifndef DISPLAY
#define DISPLAY

#define RECORDS_BY_PAGE 6

#include "memory.h"
#include "ssd1306.h"
#include <stdint.h>

void DISPLAY_Init();
void DISPLAY_WriteStart();
void DISPLAY_ScrollWrite();
void DISPLAY_WritePage(uint8_t mode, uint16_t page);
void DISPLAY_On();
void DISPLAY_Off();

#endif
