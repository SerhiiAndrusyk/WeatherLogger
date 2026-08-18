#ifndef SSD1306_BITMAPS_H
#define SSD1306_BITMAPS_H

#include <stdint.h>

typedef struct{
    uint16_t width;
    uint16_t height;
    const uint8_t *data;
} SSD1306_Bitmap;

extern const SSD1306_Bitmap SSD1306_BitmapSmile;
extern const SSD1306_Bitmap SSD1306_BitmapSun;
extern const SSD1306_Bitmap SSD1306_BitmapCloud;
extern const SSD1306_Bitmap SSD1306_BitmapPartlyCloudy;

#endif
