#ifndef STATUS_H
#define STATUS_H

#include "system_state.h"

temperatura_status_t status_da_temperatura(float temperatura_c);
float velocidade_automatica_da_temperatura(float temperatura_c);
const char *status_da_string(temperatura_status_t status);
const char *modo_da_string(modo_operacao_t modo);

#endif
