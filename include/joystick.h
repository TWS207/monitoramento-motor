#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>
#include "system_state.h"

typedef struct {
    uint16_t eixo_x_bruto;
    uint16_t eixo_y_bruto;
    float temperatura_c;
    float velocidade_manual_percentual;
} joystick_data_t;

void joystick_init(void);
joystick_data_t joystick_read(void);

#endif
