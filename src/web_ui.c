#include "web_ui.h"
#include "status.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "app.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"

//Aqui voce deve configurar o nome e senha da sua rede Wi-Fi.
#define WIFI_SSID "ELIZEU"//Nome da rede Wi-Fi a ser conectada

#define WIFI_PASSWORD "elementos"//Senha da rede Wi-Fi a ser conectada

// Ponteiro global para o servidor HTTP em escuta na porta 80.
static struct tcp_pcb *servidor_tcp = NULL;

// Guarda o progresso de uma resposta HTTP enviada em partes para economizar memoria.
typedef struct {
    bool ativo;
    struct tcp_pcb *pcb;
    char cabecalho[256];
    char corpo_copiado[256];
    size_t cabecalho_len;
    size_t cabecalho_enviado;
    const char *corpo;
    size_t corpo_len;
    size_t corpo_enviado;
} resposta_http_t;

// Como o firmware atende uma conexao por vez, um unico estado de resposta basta.
static resposta_http_t resposta_ativa = {0};

// Remove callbacks registrados no cliente antes de fechar ou abortar a conexao.
static void limpar_callbacks_cliente(struct tcp_pcb *cliente) {
    tcp_arg(cliente, NULL);
    tcp_sent(cliente, NULL);
    tcp_recv(cliente, NULL);
    tcp_err(cliente, NULL);
    tcp_poll(cliente, NULL, 0);
}

// Fecha a conexao TCP de forma segura e limpa o estado associado a resposta atual.
static err_t fechar_cliente(struct tcp_pcb *cliente) {
    if (resposta_ativa.pcb == cliente) {
        memset(&resposta_ativa, 0, sizeof(resposta_ativa));
    }
    limpar_callbacks_cliente(cliente);
    err_t erro = tcp_close(cliente);
    if (erro != ERR_OK) {
        tcp_abort(cliente);
    }
    return ERR_OK;
}

// Callback de erro do lwIP: apenas zera o estado da resposta se a conexao cair.
static void ao_erro_cliente(void *arg, err_t erro) {
    resposta_http_t *resposta = (resposta_http_t *)arg;
    (void)erro;
    if (resposta != NULL) {
        memset(resposta, 0, sizeof(*resposta));
    }
}

// Pagina HTML servida em GET /. Fica em string estatica para evitar acesso a arquivos.
static const char pagina_html[] =
"<!DOCTYPE html>"
"<html lang='pt-BR'>"
"<head>"
"  <meta charset='UTF-8'>"
"  <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"  <title>Painel de Monitoramento</title>"
"  <style>"
"    body { margin: 0; padding: 18px; font-family: Arial, sans-serif; background: linear-gradient(135deg, #f4efe7, #dbe8ea); color: #21313a; }"
"    .card { max-width: 460px; margin: 0 auto; padding: 18px; border-radius: 18px; background: rgba(255,255,255,.94); box-shadow: 0 14px 34px rgba(20,40,60,.14); }"
"    h1 { margin: 0; font-size: 1.5rem; }"
"    .sub { margin: 6px 0 16px; color: #60717b; font-size: .95rem; }"
"    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }"
"    .bloco { padding: 12px; border-radius: 14px; background: #f8fafb; border: 1px solid #d7e0e3; }"
"    .rotulo { display: block; margin-bottom: 6px; color: #60717b; font-size: .82rem; text-transform: uppercase; }"
"    .valor { font-size: 1.15rem; font-weight: bold; }"
"    .status { display: inline-block; padding: 4px 10px; border-radius: 999px; background: #edf2f4; }"
"    .barras { margin-top: 14px; }"
"    .barra-linha { margin-top: 10px; }"
"    .barra-topo { display: flex; justify-content: space-between; margin-bottom: 6px; font-size: .88rem; color: #60717b; }"
"    .barra-fundo { height: 10px; border-radius: 999px; background: #dbe5e8; overflow: hidden; }"
"    .barra-valor { height: 100%; width: 0%; border-radius: 999px; background: linear-gradient(90deg, #0f766e, #134e4a); }"
"    .rodape { margin-top: 14px; text-align: center; color: #60717b; font-size: .82rem; }"
"  </style>"
"</head>"
"<body>"
"  <div class='card'>"
"    <h1>Painel de Monitoramento</h1>"
"    <div class='sub'>Leituras em tempo real do display</div>"
"    <div class='grid'>"
"      <div class='bloco'><span class='rotulo'>Temperatura</span><span class='valor' id='temperatura'>--</span></div>"
"      <div class='bloco'><span class='rotulo'>Velocidade</span><span class='valor' id='velocidade'>--</span></div>"
"      <div class='bloco'><span class='rotulo'>Status</span><span class='valor status' id='status'>--</span></div>"
"      <div class='bloco'><span class='rotulo'>Modo</span><span class='valor' id='modo'>--</span></div>"
"    </div>"
"    <div class='barras'>"
"      <div class='barra-linha'>"
"        <div class='barra-topo'><span>Temperatura</span><span id='temp-barra-label'>0%</span></div>"
"        <div class='barra-fundo'><div class='barra-valor' id='temp-barra'></div></div>"
"      </div>"
"      <div class='barra-linha'>"
"        <div class='barra-topo'><span>Velocidade</span><span id='vel-barra-label'>0%</span></div>"
"        <div class='barra-fundo'><div class='barra-valor' id='vel-barra'></div></div>"
"      </div>"
"    </div>"
"    <div class='rodape'>Atualizacao automatica a cada 1 segundo</div>"
"  </div>"
"  <script>"
"    function limitarBarra(valor) {"
"      if (valor < 0) return 0;"
"      if (valor > 100) return 100;"
"      return valor;"
"    }"
"    function pintarStatus(texto) {"
"      const el = document.getElementById('status');"
"      const valor = String(texto || '');"
"      const normalizado = valor.toUpperCase();"
"      let fundo = '#edf2f4';"
"      let cor = '#21313a';"
"      if (normalizado.indexOf('NORMAL') >= 0) { fundo = '#e8f7ee'; cor = '#15803d'; }"
"      else if (normalizado.indexOf('CUIDADO') >= 0 || normalizado.indexOf('ALERTA') >= 0) { fundo = '#fff4df'; cor = '#f56b01'; }"
"      else if (normalizado.indexOf('CRIT') >= 0) { fundo = '#fde8e8'; cor = '#b91c1c'; }"
"      el.textContent = valor;"
"      el.style.background = fundo;"
"      el.style.color = cor;"
"    }"
"    async function atualizarEstado() {"
"      const resposta = await fetch('/estado');"
"      const dados = await resposta.json();"
"      const tempBarra = limitarBarra(dados.temperatura_c);"
"      const velBarra = limitarBarra(dados.velocidade_percentual);"
"      document.getElementById('temperatura').textContent = dados.temperatura_c.toFixed(1) + ' C';"
"      document.getElementById('velocidade').textContent = dados.velocidade_percentual.toFixed(1) + ' %';"
"      document.getElementById('modo').textContent = dados.modo;"
"      document.getElementById('temp-barra').style.width = tempBarra + '%';"
"      document.getElementById('vel-barra').style.width = velBarra + '%';"
"      document.getElementById('temp-barra-label').textContent = tempBarra.toFixed(0) + '%';"
"      document.getElementById('vel-barra-label').textContent = velBarra.toFixed(0) + '%';"
"      pintarStatus(dados.status);"
"    }"
"    setInterval(atualizarEstado, 1000);"
"    atualizarEstado();"
"  </script>"
"</body>"
"</html>";

// Envia primeiro o cabecalho HTTP e depois o corpo em blocos pequenos, conforme o
// buffer TCP vai liberando espaco. Isso evita ERR_MEM em respostas maiores.
static err_t enviar_pendente(resposta_http_t *resposta) {
    while (resposta->cabecalho_enviado < resposta->cabecalho_len) {
        u16_t disponivel = tcp_sndbuf(resposta->pcb);
        size_t restante = resposta->cabecalho_len - resposta->cabecalho_enviado;
        u16_t fatia = (u16_t)((restante < disponivel) ? restante : disponivel);
        err_t erro;

        if (fatia == 0) {
            return ERR_OK;
        }

        erro = tcp_write(
            resposta->pcb,
            resposta->cabecalho + resposta->cabecalho_enviado,
            fatia,
            TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE
        );
        if (erro != ERR_OK) {
            return erro;
        }
        resposta->cabecalho_enviado += fatia;
    }

    while (resposta->corpo_enviado < resposta->corpo_len) {
        u16_t disponivel = tcp_sndbuf(resposta->pcb);
        size_t restante = resposta->corpo_len - resposta->corpo_enviado;
        u16_t fatia = (u16_t)((restante < disponivel) ? restante : disponivel);
        err_t erro;
        u8_t flags = 0;

        if (fatia == 0) {
            return tcp_output(resposta->pcb);
        }

        if ((resposta->corpo_enviado + fatia) < resposta->corpo_len) {
            flags |= TCP_WRITE_FLAG_MORE;
        }

        erro = tcp_write(
            resposta->pcb,
            resposta->corpo + resposta->corpo_enviado,
            fatia,
            flags
        );
        if (erro != ERR_OK) {
            return erro;
        }
        resposta->corpo_enviado += fatia;
    }

    return tcp_output(resposta->pcb);
}

// Chamado quando o lwIP confirma envio de parte dos dados; tenta continuar a resposta.
static err_t ao_enviar_cliente(void *arg, struct tcp_pcb *cliente, u16_t len) {
    resposta_http_t *resposta = (resposta_http_t *)arg;
    (void)len;

    if (resposta == NULL || resposta->pcb != cliente || !resposta->ativo) {
        return fechar_cliente(cliente);
    }

    err_t erro = enviar_pendente(resposta);
    if (erro != ERR_OK) {
        printf("Falha ao continuar resposta HTTP (erro %d)\n", erro);
        return fechar_cliente(cliente);
    }

    if (resposta->cabecalho_enviado >= resposta->cabecalho_len &&
        resposta->corpo_enviado >= resposta->corpo_len) {
        return fechar_cliente(cliente);
    }

    return ERR_OK;
}

// Reenvio de seguranca: se ainda houver dados pendentes, tenta avancar no proximo poll.
static err_t ao_poll_cliente(void *arg, struct tcp_pcb *cliente) {
    resposta_http_t *resposta = (resposta_http_t *)arg;

    if (resposta == NULL || resposta->pcb != cliente || !resposta->ativo) {
        return fechar_cliente(cliente);
    }

    err_t erro = enviar_pendente(resposta);
    if (erro != ERR_OK) {
        printf("Falha ao reenviar resposta HTTP (erro %d)\n", erro);
        return fechar_cliente(cliente);
    }

    if (resposta->cabecalho_enviado >= resposta->cabecalho_len &&
        resposta->corpo_enviado >= resposta->corpo_len) {
        return fechar_cliente(cliente);
    }

    return ERR_OK;
}

// Prepara o cabecalho HTTP e registra os callbacks necessarios para transmitir
// a resposta completa sem bloquear o loop principal.
static err_t iniciar_resposta(struct tcp_pcb *cliente, const char *tipo, const char *corpo, bool copiar_corpo) {
    int tamanho_corpo = (int)strlen(corpo);

    memset(&resposta_ativa, 0, sizeof(resposta_ativa));
    resposta_ativa.ativo = true;
    resposta_ativa.pcb = cliente;
    if (copiar_corpo) {
        if ((size_t)tamanho_corpo >= sizeof(resposta_ativa.corpo_copiado)) {
            return ERR_MEM;
        }
        memcpy(resposta_ativa.corpo_copiado, corpo, (size_t)tamanho_corpo + 1);
        resposta_ativa.corpo = resposta_ativa.corpo_copiado;
    } else {
        resposta_ativa.corpo = corpo;
    }
    resposta_ativa.corpo_len = (size_t)tamanho_corpo;
    resposta_ativa.cabecalho_len = (size_t)snprintf(
        resposta_ativa.cabecalho,
        sizeof(resposta_ativa.cabecalho),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n",
        tipo,
        tamanho_corpo
    );

    tcp_arg(cliente, &resposta_ativa);
    tcp_sent(cliente, ao_enviar_cliente);
    tcp_err(cliente, ao_erro_cliente);
    tcp_poll(cliente, ao_poll_cliente, 2);
    return enviar_pendente(&resposta_ativa);
}

// Monta o JSON consumido pelo frontend com os mesmos campos mostrados no display.
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
        "\"modo\":\"%s\""
        "}",
        estado.temperatura_c,
        estado.velocidade_percentual,
        status_da_string(estado.status),
        modo_da_string(estado.modo)
    );
}

// Processa uma requisicao HTTP recebida do navegador e escolhe qual conteudo enviar.
static err_t ao_receber(
    void *arg,
    struct tcp_pcb *cliente,
    struct pbuf *buffer,
    err_t erro
) {
    (void)arg;
    (void)erro;

    if (buffer == NULL) {
        fechar_cliente(cliente);
        return ERR_OK;
    }

    char requisicao[1024] = {0};
    uint16_t tamanho_requisicao = buffer->tot_len;
    if (tamanho_requisicao >= sizeof(requisicao)) {
        tamanho_requisicao = sizeof(requisicao) - 1;
    }

    pbuf_copy_partial(buffer, requisicao, tamanho_requisicao, 0);
    tcp_recved(cliente, buffer->tot_len);
    pbuf_free(buffer);

    err_t erro_resposta;
    if (strncmp(requisicao, "GET / ", 6) == 0) {
        erro_resposta = iniciar_resposta(cliente, "text/html; charset=UTF-8", pagina_html, false);
    } else if (strncmp(requisicao, "GET /estado ", 12) == 0) {
        char json[192];
        montar_json_estado(json, sizeof(json));
        erro_resposta = iniciar_resposta(cliente, "application/json", json, true);
    } else {
        erro_resposta = iniciar_resposta(cliente, "text/plain", "Rota nao encontrada", false);
    }

    if (erro_resposta != ERR_OK) {
        printf("Falha ao enviar resposta HTTP (erro %d)\n", erro_resposta);
        fechar_cliente(cliente);
    }
    return ERR_OK;
}

// Associa o callback de recebimento a cada novo cliente aceito pelo servidor.
static err_t ao_aceitar(void *arg, struct tcp_pcb *cliente, err_t erro) {
    (void)arg;
    if (erro != ERR_OK || cliente == NULL) {
        return ERR_VAL;
    }
    if (resposta_ativa.ativo) {
        return ERR_MEM;
    }
    tcp_arg(cliente, &resposta_ativa);
    tcp_err(cliente, ao_erro_cliente);
    tcp_recv(cliente, ao_receber);
    return ERR_OK;
}

// Inicializa o Wi-Fi, conecta na rede configurada e sobe o servidor HTTP.
void web_ui_init(void) {
    if (cyw43_arch_init()) {
        printf("Falha ao inicializar o Wi-Fi\n");
        return;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi '%s'...\n", WIFI_SSID);

    if (cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID,
            WIFI_PASSWORD,
            CYW43_AUTH_WPA2_AES_PSK,
            30000) != 0) {
        printf("Falha ao conectar com Wi-Fi\n");
        return;
    }

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
        printf("Servidor web rodando em http://%s/\n",
               ip4addr_ntoa(netif_ip4_addr(netif_default)));
    } else {
        printf("Infelizmente nao foi possivel obter endereco IP\n");
    }
}

// No modo lwIP poll, esta chamada precisa acontecer no loop principal para a
// pilha de rede continuar processando eventos e conexoes.
void web_ui_poll(void) {
    cyw43_arch_poll();
}
