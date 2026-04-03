#ifndef CONFIG_H
#define CONFIG_H

#include "hardware/adc.h"
#include "hardware/i2c.h"

// LED RGB
#define LED_VERMELHO_PIN 13
#define LED_AZUL_PIN 12
#define LED_VERDE_PIN 11

// OLED SSD1306 via I2C
#define OLED_I2C i2c1
#define OLED_SDA_PIN 14
#define OLED_SCL_PIN 15
#define OLED_ADDRESS 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// Buttons
#define BOTAO_A_PIN 5
#define BOTAO_B_PIN 6

// Motor output
#define MOTOR_PWM_PIN 4

// Buzzer onboard
#define BUZZER_PWM_PIN 21
#define BUZZER_FREQUENCIA_HZ 2000

// Joystick
#define JOYSTICK_Y_PIN 26
#define JOYSTICK_X_PIN 27
#define JOYSTICK_SW_PIN 22
#define JOYSTICK_Y_ADC_INPUT 0
#define JOYSTICK_X_ADC_INPUT 1

// Application timing
#define APP_LOOP_DELAY_MS 10
#define OLED_LEITURA_MS 250
#endif
