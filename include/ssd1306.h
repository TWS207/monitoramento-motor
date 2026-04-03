#ifndef SSD1306_H
#define SSD1306_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_draw_text(uint8_t x, uint8_t y, const char *texto);
void ssd1306_draw_rect(uint8_t x, uint8_t y, uint8_t largura, uint8_t altura, bool preenchido);
void ssd1306_present(void);

#endif
