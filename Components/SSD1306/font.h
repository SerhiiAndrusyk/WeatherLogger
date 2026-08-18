#ifndef SSD1306_FONT_H
#define SSD1306_FONT_H

#include <stdint.h>

typedef struct{
    uint8_t width;
    uint8_t height;
    char firstCharacter;
    char lastCharacter;
    const uint8_t *data;
} SSD1306_Font;

extern const SSD1306_Font SSD1306_Font5x7;

#endif
