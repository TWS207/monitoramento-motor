 `Este projeto roda na BitDogLab/Pico W e faz um monitoramento de temperatura com interface local e web. O sistema lê os valores do joystick/sensor, calcula o estado atual da temperatura, define a velocidade do motor conforme o modo de operação e mostra essas informações no display OLED.`

`Além da parte física, o firmware conecta a placa ao Wi‑Fi e sobe um pequeno servidor HTTP. Esse servidor entrega uma página web leve que mostra, em tempo real, os mesmos dados exibidos no display: temperatura, velocidade, status e modo. O loop principal mantém tudo atualizado continuamente, cuidando das leituras, do display, das saídas e da comunicação de rede.`

## Objetivo
- O projeto tem como objetivo realizar o monitoramento em tempo real da simulação de um motor, destacando as principais variáveis para garantir um funcionamento seguro e eficiente, especialmente temperatura e velocidade. O sistema oferece duas formas de acompanhamento: localmente, por meio do display da própria placa, permitindo visualização e interação direta com o sistema; e remotamente, através de um servidor HTTP, possibilitando ao operador monitorar o comportamento do motor à distância e identificar alterações no funcionamento.


## Mapeamento de hardware usado

- LED RGB: `GPIO13` vermelho, `GPIO12` azul, `GPIO11` verde
- OLED SSD1306 I2C: `GPIO14` SDA, `GPIO15` SCL
- Botao manual: `GPIO5`
- Botao automatico: `GPIO6`
- Saida PWM do motor: `GPIO4`
- Buzzer onboard: `GPIO21`
- Joystick: `Y GPIO26/ADC0`, `X GPIO27/ADC1`, `SW GPIO22`


- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


- Eixo Y do joystick: temperatura de `0 a 100 C`
- Eixo X do joystick: velocidade manual do motor de `0 a 100 %`
- Faixas de temperatura:
  - `< 70 C`: `NORMAL`
  - `70 a 87 C`: `ATENCAO`
  - `> 87 C`: `CRITICO`
- Modo `MANUAL`: velocidade vem do eixo X
- Modo `AUTO`: velocidade e definida pela temperatura
  - `NORMAL`: `35 %`
  - `ATENCAO`: `65 %`
  - `CRITICO`: `100 %`
- Buzzer:
  - `NORMAL`: desligado
  - `ATENCAO`: beep curto periodico
  - `CRITICO`: beep rapido continuo
- Interface local no display OLED:
  - exibicao de temperatura
  - exibicao de velocidade
  - exibicao de status
  - exibicao do modo de operacao

## Estrutura do firmware RP2040

- `src/app.c`: orquestra o loop principal e as interrupcoes
- `src/joystick.c`: leitura ADC e conversao
- `src/status.c`: classificacao e regras de controle automatico
- `src/outputs.c`: LED RGB, PWM do motor e buzzer
- `src/display.c`: composicao da tela OLED
- `src/ssd1306.c`: driver basico do display
- `src/web_ui.c`: servidor HTTP e configurações de rede, juntamente com `lwipopts.h`


## Como usar

1. Grave o firmware na `BITDOGLAB`, conecte o cabo micro USB na sua `BITDOGLAB` e antes de conectar ao computador certifique-se se segurar o botão BOOTSEL atrás da placa, após isso basta dar compilar e rodar o codigo.
2. Acompanhe o estado do sistema diretamente no display OLED ou no Site.
3. Use os botoes fisicos para alternar entre `MANUAL` e `AUTO`.
4. Para conectar a placa com a sua rede local vá em `src/web_ui.c` lá você ira se deparar com dois #define, Um WIFI_SSID e nele colocará o nome da Sua rede e logo abaixo WIFI_PASSWORD e nele colocará sua Senha.
5. Para ter acesso ao seu URL do site, abra o Monitor Serial e Vá na entrada em que sua BitDogLab está Conectada, faça isso antes da Sua Placa Carregar Completamente que lá vai apaecer o link de Acesso
6. Agora está pronto para fazer o monitoramento!!


                FLUXOGRAMA
                    |
                    |
                    V

[Início]
   ↓
[Inicializa sistema]
 (Wi-Fi, sensores, display, servidor HTTP)
   ↓
[Conecta ao Wi-Fi]
   ↓
[Inicia servidor web]
   ↓
[Loop principal]
   ↓
[Leitura dos sensores]
 (Temperatura e Velocidade)
   ↓
[Atualiza estado do sistema]
   ↓
[Define status]
 (NORMAL / ATENÇÃO / CRÍTICO)
   ↓
[Atualiza display da placa]
   ↓
[Atualiza dados para servidor HTTP]
   ↓
[Retorna resposta ao cliente]
   ↓
(Volta para o loop)