#ifndef DISPLAY
#define DISPLAY

#include "w25q128.h"
#include "ssd1306.h"

void DISPLAY_Init();
void DISPLAY_WriteStart();
void DISPLAY_ScrollWrite();
void DISPLAY_WritePage(uint8_t mode, uint8_t page);

#endif
