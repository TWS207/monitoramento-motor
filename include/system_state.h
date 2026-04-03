#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    STATUS_NORMAL = 0,
    STATUS_CUIDADO,
    STATUS_CRITICO
} temperatura_status_t;

typedef enum {
    MODO_MANUAL = 0,
    MODO_AUTOMATICO
} modo_operacao_t;

typedef struct {
    uint16_t eixo_x_bruto;
    uint16_t eixo_y_bruto;
    float temperatura_c;
    float velocidade_percentual;
    float velocidade_manual_percentual;
    temperatura_status_t status;
    modo_operacao_t modo;
    bool motor_habilitado;
    bool velocidade_manual_habilitada;
} system_state_t;

#endif
