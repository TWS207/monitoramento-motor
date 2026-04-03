#include "status.h"

temperatura_status_t status_da_temperatura(float temperatura_c) {
    // Classifica a temperatura em faixas para facilitar a reação do sistema.
    if (temperatura_c < 70.0f) {
        return STATUS_NORMAL;
    }
    if (temperatura_c <= 88.0f) {
        return STATUS_CUIDADO;
    }
    return STATUS_CRITICO;
}

float velocidade_automatica_da_temperatura(float temperatura_c) {
    // Define a velocidade do motor automaticamente conforme a temperatura.
    if (temperatura_c < 50.0f) {
        return 35.0f;
    }
    if (temperatura_c <= 75.0f) {
        return 65.0f;
    }
    return 100.0f;
}

const char *status_da_string(temperatura_status_t status) {
    // Converte o enum em texto para uso no display e em futuras interfaces.
    switch (status) {
        case STATUS_NORMAL:
            return "NORMAL";
        case STATUS_CUIDADO:
            return "ATENCAO";
        case STATUS_CRITICO:
        default:
            return "CRITICO";
    }
}

const char *modo_da_string(modo_operacao_t modo) {
    // Resume o modo em um texto curto para exibição.
    return (modo == MODO_AUTOMATICO) ? "AUTO" : "MANUAL";
}
