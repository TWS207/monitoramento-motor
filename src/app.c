#include "app.h"

#include <stdbool.h>
#include "config.h"
#include "display.h"
#include "hardware/gpio.h"
#include "joystick.h"
#include "outputs.h"
#include "pico/stdlib.h"
#include "status.h"
#include "system_state.h"

static volatile modo_operacao_t pedir_modo = MODO_MANUAL;
static volatile bool pedir_motor_habilitado = true;
static volatile bool velocidade_manual_habilitada = false;
static volatile float velocidade_manual_percentual = 0.0f;
static system_state_t estado_atual;

static void tratar_interrupcao_botao(uint gpio, uint32_t eventos) {
    (void)eventos;
    if (gpio == BOTAO_A_PIN) {
        pedir_modo = MODO_MANUAL;
    } else if (gpio == BOTAO_B_PIN) {
        pedir_modo = MODO_AUTOMATICO;
    }
}

static void iniciar_botoes(void) {
    gpio_init(BOTAO_A_PIN);
    gpio_set_dir(BOTAO_A_PIN, GPIO_IN);
    gpio_pull_up(BOTAO_A_PIN);

    gpio_init(BOTAO_B_PIN);
    gpio_set_dir(BOTAO_B_PIN, GPIO_IN);
    gpio_pull_up(BOTAO_B_PIN);

    gpio_set_irq_enabled_with_callback(BOTAO_A_PIN, GPIO_IRQ_EDGE_FALL, true, &tratar_interrupcao_botao);
    gpio_set_irq_enabled(BOTAO_B_PIN, GPIO_IRQ_EDGE_FALL, true);
}

static void atualizar_estado_das_entradas(void) {
    joystick_data_t leitura_joystick = joystick_read();

    estado_atual.eixo_x_bruto = leitura_joystick.eixo_x_bruto;
    estado_atual.eixo_y_bruto = leitura_joystick.eixo_y_bruto;
    estado_atual.temperatura_c = leitura_joystick.temperatura_c;
    estado_atual.modo = pedir_modo;
    estado_atual.status = status_da_temperatura(estado_atual.temperatura_c);
    estado_atual.motor_habilitado = pedir_motor_habilitado;
    estado_atual.velocidade_manual_habilitada = velocidade_manual_habilitada;
    estado_atual.velocidade_manual_percentual = velocidade_manual_percentual;

    // No modo automatico, a velocidade segue a estrategia definida pela faixa
    // de temperatura. No modo manual, o eixo X do joystick domina o controle.
    if (estado_atual.modo == MODO_AUTOMATICO) {
        estado_atual.velocidade_percentual = velocidade_automatica_da_temperatura(estado_atual.temperatura_c);
    } else if (estado_atual.velocidade_manual_habilitada) {
        estado_atual.velocidade_percentual = estado_atual.velocidade_manual_percentual;
    } else {
        estado_atual.velocidade_percentual = leitura_joystick.velocidade_manual_percentual;
    }
}

void app_init(void) {
    stdio_init_all();
    joystick_init();
    outputs_iniciar();
    display_init();
    iniciar_botoes();

    estado_atual.modo = MODO_MANUAL;
    pedir_modo = MODO_MANUAL;
}

void app_run_forever(void) {
    while (true) {
        atualizar_estado_das_entradas();

        uint32_t ms_atual = to_ms_since_boot(get_absolute_time());
        // O loop atualiza as saidas locais e a interface a cada iteracao.
        outputs_aplicar(&estado_atual, ms_atual);
        display_render(&estado_atual, ms_atual);

        sleep_ms(APP_LOOP_DELAY_MS);
    }
}

void app_pegar_estado(system_state_t *out_estado) {
    if (out_estado == NULL) {
        return;
    }
    *out_estado = estado_atual;
}

void app_pedir_modo(modo_operacao_t modo) {
    pedir_modo = modo;
}

void app_setar_motor_habilitado(bool habilitado) {
    pedir_motor_habilitado = habilitado;
}

void app_setar_velocidade_manual(float velocidade_percentual) {
    if (velocidade_percentual < 0.0f) {
        velocidade_percentual = 0.0f;
    }
    if (velocidade_percentual > 100.0f) {
        velocidade_percentual = 100.0f;
    }
    velocidade_manual_percentual = velocidade_percentual;
    velocidade_manual_habilitada = true;
}

void app_limpar_velocidade_manual(void) {
    velocidade_manual_habilitada = false;
}
