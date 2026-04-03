#include "status.h"

temperatura_status_t status_da_temperatura(float temperatura_c) {
    if (temperatura_c < 50.0f) {
        return STATUS_NORMAL;
    }
    if (temperatura_c <= 75.0f) {
        return STATUS_CUIDADO;
    }
    return STATUS_CRITICO;
}

float velocidade_automatica_da_temperatura(float temperatura_c) {
    if (temperatura_c < 50.0f) {
        return 35.0f;
    }
    if (temperatura_c <= 75.0f) {
        return 65.0f;
    }
    return 100.0f;
}

const char *status_da_string(temperatura_status_t status) {
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
    return (modo == MODO_AUTOMATICO) ? "AUTO" : "MANUAL";
}
