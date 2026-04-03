#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <stdbool.h>
#include "system_state.h"

void outputs_iniciar(void);
void outputs_aplicar(const system_state_t *estado, uint32_t ms_atual);
void outputs_velocidade_motor(float velocidade_percentual);
void outputs_set_rgb(temperatura_status_t status);

#endif
