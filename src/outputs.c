#include "outputs.h"

#include <stdint.h>
#include "config.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

static uint fatia_motor;
static uint canal_motor;
static uint fatia_buzzer;
static uint canal_buzzer;
static uint16_t limite_buzzer;

static uint16_t percentual_para_nivel(float percentual) {
    if (percentual <= 0.0f) {
        return 0;
    }
    if (percentual >= 100.0f) {
        return 65535;
    }
    return (uint16_t)((percentual / 100.0f) * 65535.0f);
}

static void buzzer_setar_habilitado(bool habilitado) {
    pwm_set_chan_level(fatia_buzzer, canal_buzzer, habilitado ? (limite_buzzer / 2u) : 0u);
}

static void outputs_setar_buzzer(temperatura_status_t status, uint32_t ms_atual) {
    if (status == STATUS_NORMAL) {
        buzzer_setar_habilitado(false);
        return;
    }

    if (status == STATUS_CUIDADO) {
        buzzer_setar_habilitado((ms_atual % 1000u) < 200u);
        return;
    }

    buzzer_setar_habilitado((ms_atual % 300u) < 150u);
}

void outputs_iniciar(void) {
    gpio_init(LED_VERMELHO_PIN);
    gpio_set_dir(LED_VERMELHO_PIN, GPIO_OUT);
    gpio_init(LED_VERDE_PIN);
    gpio_set_dir(LED_VERDE_PIN, GPIO_OUT);
    gpio_init(LED_AZUL_PIN);
    gpio_set_dir(LED_AZUL_PIN, GPIO_OUT);

    // A saida do motor entrega um duty cycle de 0..100% para um driver externo.
    gpio_set_function(MOTOR_PWM_PIN, GPIO_FUNC_PWM);
    fatia_motor = pwm_gpio_to_slice_num(MOTOR_PWM_PIN);
    canal_motor = pwm_gpio_to_channel(MOTOR_PWM_PIN);
    pwm_set_wrap(fatia_motor, 65535);
    pwm_set_clkdiv(fatia_motor, 4.0f);
    pwm_set_chan_level(fatia_motor, canal_motor, 0);
    pwm_set_enabled(fatia_motor, true);

    gpio_set_function(BUZZER_PWM_PIN, GPIO_FUNC_PWM);
    fatia_buzzer = pwm_gpio_to_slice_num(BUZZER_PWM_PIN);
    canal_buzzer = pwm_gpio_to_channel(BUZZER_PWM_PIN);
    limite_buzzer = (uint16_t)((clock_get_hz(clk_sys) / (64.0f * BUZZER_FREQUENCIA_HZ)) - 1.0f);
    pwm_set_wrap(fatia_buzzer, limite_buzzer);
    pwm_set_clkdiv(fatia_buzzer, 64.0f);
    pwm_set_chan_level(fatia_buzzer, canal_buzzer, 0);
    pwm_set_enabled(fatia_buzzer, true);
}

void outputs_velocidade_motor(float velocidade_percentual) {
    pwm_set_chan_level(fatia_motor, canal_motor, percentual_para_nivel(velocidade_percentual));
}

void outputs_set_rgb(temperatura_status_t status) {
    gpio_put(LED_VERMELHO_PIN, status == STATUS_CRITICO || status == STATUS_CUIDADO);
    gpio_put(LED_VERDE_PIN, status == STATUS_NORMAL || status == STATUS_CUIDADO);
    gpio_put(LED_AZUL_PIN, false);
}

void outputs_aplicar(const system_state_t *estado, uint32_t ms_atual) {
    outputs_set_rgb(estado->status);
    outputs_setar_buzzer(estado->status, ms_atual);
    outputs_velocidade_motor(estado->motor_habilitado ? estado->velocidade_percentual : 0.0f);
}
