#include "joystick.h"

#include "config.h"
#include "hardware/gpio.h"

static float escala_adc_percentual(uint16_t valor_bruto) {
    // Converte a leitura de 12 bits do ADC para a faixa percentual de 0 a 100.
    return (valor_bruto * 100.0f) / 4095.0f;
}

void joystick_init(void) {
    // GPIO26 e GPIO27 entram no ADC para leitura continua dos eixos.
    adc_init();
    adc_gpio_init(JOYSTICK_Y_PIN);
    adc_gpio_init(JOYSTICK_X_PIN);

    // O botão do joystick fica disponível para futuras expansões do projeto.
    gpio_init(JOYSTICK_SW_PIN);
    gpio_set_dir(JOYSTICK_SW_PIN, GPIO_IN);
    gpio_pull_up(JOYSTICK_SW_PIN);
}

joystick_data_t joystick_read(void) {
    joystick_data_t dados = {0};

    // O eixo Y representa a temperatura simulada.
    adc_select_input(JOYSTICK_Y_ADC_INPUT);
    dados.eixo_y_bruto = adc_read();

    // O eixo X representa a velocidade manual.
    adc_select_input(JOYSTICK_X_ADC_INPUT);
    dados.eixo_x_bruto = adc_read();

    // A conversao segue o enunciado: ambos os eixos sao mapeados para 0..100.
    dados.temperatura_c = escala_adc_percentual(dados.eixo_y_bruto);
    dados.velocidade_manual_percentual = escala_adc_percentual(dados.eixo_x_bruto);
    return dados;
}
