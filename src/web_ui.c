#include "web_ui.h"
#include "status.h"
#include <stdio.h>
#include <string.h>
#include "app.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"


#define WIFI_SSID "ELIZEU"
#define WIFI_PASSWORD "elementos"

static struct tcp_pcb *servidor_tcp = NULL;

static const char pagina_html[] =
"<!DOCTYPE html>"
"<html lang='pt-BR'>"
"<head>"
"  <meta charset='UTF-8'>"
"  <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"  <title>Monitoramento</title>"
"  <style>"
"    body { font-family: Arial, sans-serif; margin: 20px; background: #f4f4f4; }"
"    .card { background: white; padding: 16px; border-radius: 10px; max-width: 420px; }"
"    h1 { margin-top: 0; }"
"    .linha { margin: 8px 0; }"
"    button { margin-right: 8px; margin-top: 8px; }"
"    input[type=range] { width: 100%; }"
"  </style>"
"</head>"
"<body>"
"  <div class='card'>"
"    <h1>BitDogLab</h1>"
"    <div class='linha'>Temperatura: <span id='temperatura'>--</span></div>"
"    <div class='linha'>Velocidade: <span id='velocidade'>--</span></div>"
"    <div class='linha'>Status: <span id='status'>--</span></div>"
"    <div class='linha'>Modo: <span id='modo'>--</span></div>"
"    <div class='linha'>Motor: <span id='motor'>--</span></div>"
"    <button onclick=\"definirModo('MANUAL')\">Manual</button>"
"    <button onclick=\"definirModo('AUTOMATICO')\">Automático</button>"
"    <button onclick=\"definirMotor(true)\">Ligar motor</button>"
"    <button onclick=\"definirMotor(false)\">Desligar motor</button>"
"    <div class='linha'>"
"      <label for='vel'>Velocidade manual</label>"
"      <input id='vel' type='range' min='0' max='100' value='0' oninput='enviarVelocidade(this.value)'>"
"    </div>"
"  </div>"
"  <script>"
"    async function atualizarEstado() {"
"      const resposta = await fetch('/estado');"
"      const dados = await resposta.json();"
"      document.getElementById('temperatura').textContent = dados.temperatura_c.toFixed(1) + ' C';"
"      document.getElementById('velocidade').textContent = dados.velocidade_percentual.toFixed(1) + ' %';"
"      document.getElementById('status').textContent = dados.status;"
"      document.getElementById('modo').textContent = dados.modo;"
"      document.getElementById('motor').textContent = dados.motor_habilitado ? 'Ligado' : 'Desligado';"
"    }"
"    async function definirModo(modo) {"
"      await fetch('/modo', { method: 'POST', body: modo });"
"      atualizarEstado();"
"    }"
"    async function definirMotor(habilitado) {"
"      await fetch('/motor', { method: 'POST', body: habilitado ? '1' : '0' });"
"      atualizarEstado();"
"    }"
"    async function enviarVelocidade(valor) {"
"      await fetch('/velocidade', { method: 'POST', body: String(valor) });"
"      atualizarEstado();"
"    }"
"    setInterval(atualizarEstado, 1000);"
"    atualizarEstado();"
"  </script>"
"</body>"
"</html>";

static err_t enviar_resposta(struct tcp_pcb *cliente, const char *tipo, const char *corpo) {
    char cabecalho[256];
    int tamanho_corpo = (int)strlen(corpo);

    snprintf(
        cabecalho,
        sizeof(cabecalho),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n",
        tipo,
        tamanho_corpo
    );

    tcp_write(cliente, cabecalho, strlen(cabecalho), TCP_WRITE_FLAG_COPY);
    tcp_write(cliente, corpo, tamanho_corpo, TCP_WRITE_FLAG_COPY);
    tcp_output(cliente);
    return ERR_OK;
}

static void montar_json_estado(char *saida, size_t tamanho) {
    system_state_t estado;
    app_pegar_estado(&estado);

    snprintf(
        saida,
        tamanho,
        "{"
        "\"temperatura_c\":%.1f,"
        "\"velocidade_percentual\":%.1f,"
        "\"status\":\"%s\","
        "\"modo\":\"%s\","
        "\"motor_habilitado\":%s"
        "}",
        estado.temperatura_c,
        estado.velocidade_percentual,
        status_da_string(estado.status),
        modo_da_string(estado.modo),
        estado.motor_habilitado ? "true" : "false"
    );
}

static void tratar_post_modo(const char *corpo) {
    if (strncmp(corpo, "MANUAL", 6) == 0) {
        app_pedir_modo(MODO_MANUAL);
    } else if (strncmp(corpo, "AUTOMATICO", 10) == 0) {
        app_pedir_modo(MODO_AUTOMATICO);
    }
}

static void tratar_post_motor(const char *corpo) {
    if (corpo[0] == '1') {
        app_setar_motor_habilitado(true);
    } else {
        app_setar_motor_habilitado(false);
    }
}

static void tratar_post_velocidade(const char *corpo) {
    float velocidade = 0.0f;
    sscanf(corpo, "%f", &velocidade);
    app_setar_velocidade_manual(velocidade);
}

static err_t ao_receber(
    void *arg,
    struct tcp_pcb *cliente,
    struct pbuf *buffer,
    err_t erro
) {
    (void)arg;
    (void)erro;

    if (buffer == NULL) {
        tcp_close(cliente);
        return ERR_OK;
    }

    char requisicao[1024] = {0};
    pbuf_copy_partial(buffer, requisicao, buffer->tot_len, 0);
    pbuf_free(buffer);

    if (strncmp(requisicao, "GET / ", 6) == 0) {
        enviar_resposta(cliente, "text/html; charset=UTF-8", pagina_html);
    } else if (strncmp(requisicao, "GET /estado ", 12) == 0) {
        char json[256];
        montar_json_estado(json, sizeof(json));
        enviar_resposta(cliente, "application/json", json);
    } else if (strncmp(requisicao, "POST /modo ", 11) == 0) {
        const char *corpo = strstr(requisicao, "\r\n\r\n");
        if (corpo != NULL) {
            tratar_post_modo(corpo + 4);
        }
        enviar_resposta(cliente, "text/plain", "OK");
    } else if (strncmp(requisicao, "POST /motor ", 12) == 0) {
        const char *corpo = strstr(requisicao, "\r\n\r\n");
        if (corpo != NULL) {
            tratar_post_motor(corpo + 4);
        }
        enviar_resposta(cliente, "text/plain", "OK");
    } else if (strncmp(requisicao, "POST /velocidade ", 17) == 0) {
        const char *corpo = strstr(requisicao, "\r\n\r\n");
        if (corpo != NULL) {
            tratar_post_velocidade(corpo + 4);
        }
        enviar_resposta(cliente, "text/plain", "OK");
    } else {
        enviar_resposta(cliente, "text/plain", "Rota nao encontrada");
    }

    tcp_close(cliente);
    return ERR_OK;
}

static err_t ao_aceitar(void *arg, struct tcp_pcb *cliente, err_t erro) {
    (void)arg;
    (void)erro;
    tcp_recv(cliente, ao_receber);
    return ERR_OK;
}

void web_ui_init(void) {
    if (cyw43_arch_init()) {
//Mostra a falha de conexão com o Wi-Fi
        printf("Falha ao inicializar o Wi-Fi\n");
        return;
    }

    cyw43_arch_enable_sta_mode();
//Mostra a Conexão bem sucedida com o nome da rede Wi-Fi
    printf("Conectando ao Wi-Fi '%s'...\n", WIFI_SSID);

    if (cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID,
            WIFI_PASSWORD,
            CYW43_AUTH_WPA2_AES_PSK,
            30000) != 0) {
        printf("Falha ao conectar com Wi-Fi\n");
        return;
    }
    //conexão bem sucedida
    printf("Conectado ao Wi-Fi '%s'\n", WIFI_SSID);

    servidor_tcp = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (servidor_tcp == NULL) {
        printf("Falha ao criar o servidor TCP\n");
        return;
    }

    if (tcp_bind(servidor_tcp, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Falha ao vincular o servidor TCP na porta 80\n");
        return;
    }

    servidor_tcp = tcp_listen_with_backlog(servidor_tcp, 1);
    tcp_accept(servidor_tcp, ao_aceitar);

if (netif_default != NULL) {
        ip4_addr_t ip = netif_default->ip_addr;
        printf("Servidor web rodando em http://%s/\n",
                ip4addr_ntoa(netif_ip4_addr(netif_default)));
        }else{printf("Infelizmente não foi possivel Obter endereço\n");
    }
}

void web_ui_poll(void) {
    cyw43_arch_poll();
}
