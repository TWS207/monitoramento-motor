#ifndef LWIPOPTS_H
#define LWIPOPTS_H

// O projeto usa a arquitetura "poll", sem sistema operacional.
#define NO_SYS                          1

// As APIs de socket e netconn não são usadas neste projeto.
#define LWIP_SOCKET                     0
#define LWIP_NETCONN                    0

// Configurações básicas de memória do lwIP.
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        4000
#define MEM_LIBC_MALLOC                 0
#define MEMP_MEM_MALLOC                 0

// Recursos de rede usados pelo projeto.
#define LWIP_TCP                        1
#define LWIP_IPV4                       1
#define LWIP_ARP                        1
#define LWIP_ICMP                       1
#define LWIP_DHCP                       1
#define LWIP_DNS                        1
#define LWIP_RAW                        1

// Ajustes básicos do TCP.
#define TCP_MSS                         1460
#define TCP_WND                         (8 * TCP_MSS)
#define TCP_SND_BUF                     (8 * TCP_MSS)
#define TCP_SND_QUEUELEN                32
#define MEMP_NUM_TCP_SEG                32
#define TCP_LISTEN_BACKLOG              1

// Mantém a configuração mais simples para firmware bare-metal.
#define SYS_LIGHTWEIGHT_PROT            0
#define LWIP_STATS                      0

#endif
