#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include "system_state.h"

void display_init(void);
void display_render(const system_state_t *estado, uint32_t tempo_atual_ms);

#endif
