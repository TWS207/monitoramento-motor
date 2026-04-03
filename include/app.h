#ifndef APP_H
#define APP_H

#include <stdbool.h>
#include "system_state.h"

void app_init(void);
void app_run_forever(void);
void app_pegar_estado(system_state_t *out_estado);
void app_pedir_modo(modo_operacao_t modo);
void app_setar_motor_habilitado(bool habilitado);
void app_setar_velocidade_manual(float velocidade_percentual);
void app_limpar_velocidade_manual(void);

#endif
