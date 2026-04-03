#include "display.h"

#include <stdio.h>
#include "config.h"
#include "pico/time.h"
#include "ssd1306.h"
#include "status.h"

// Controla o intervalo mínimo entre atualizações do OLED.
static uint32_t ultima_varredura_ms;

static uint8_t limitar_largura_barra(float valor) {
    // Limita o valor exibido nas barras gráficas ao intervalo de 0 a 100.
    if (valor <= 0.0f) {
        return 0;
    }
    if (valor >= 100.0f) {
        return 100;
    }
    return (uint8_t)valor;
}

void display_init(void) {
    // Mostra uma tela simples de inicialização ao ligar o sistema.
    ssd1306_init();
    ssd1306_clear();
    ssd1306_draw_text(0, 0, "   BITDOGLAB ON");
    ssd1306_draw_text(0, 16, "CARREGANDO...");
    ssd1306_present();
    ultima_varredura_ms = 0;
}

void display_render(const system_state_t *estado, uint32_t tempo_atual_ms) {
    // Evita redesenhar o display mais rápido do que o necessário.
    if ((tempo_atual_ms - ultima_varredura_ms) < OLED_LEITURA_MS) {
        return;
    }

    char linha[24];
    ssd1306_clear();

    snprintf(linha, sizeof(linha), "TEMP: %5.1f C", estado->temperatura_c);
    ssd1306_draw_text(0, 0, linha);

    snprintf(linha, sizeof(linha), "VEL : %5.1f %%", estado->velocidade_percentual);
    ssd1306_draw_text(0, 10, linha);

    snprintf(linha, sizeof(linha), "STATUS: %s", status_da_string(estado->status));
    ssd1306_draw_text(0, 20, linha);

    snprintf(linha, sizeof(linha), "MODO: %s", modo_da_string(estado->modo));
    ssd1306_draw_text(0, 30, linha);

    // As barras ajudam a visualizar rapidamente temperatura e velocidade.
    ssd1306_draw_text(0, 42, "TEMP");
    ssd1306_draw_rect(28, 42, 100, 8, false);
    ssd1306_draw_rect(28, 42, limitar_largura_barra(estado->temperatura_c), 8, true);

    ssd1306_draw_text(0, 54, "VEL");
    ssd1306_draw_rect(28, 54, 100, 8, false);
    ssd1306_draw_rect(28, 54, limitar_largura_barra(estado->velocidade_percentual), 8, true);

    ssd1306_present();
    ultima_varredura_ms = tempo_atual_ms;
}
