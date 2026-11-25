/* ============================================================
 *  net.c — добавление RAW-сокета
 * ============================================================ */

#include "net.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>

int raw_socket_create(const char* ifname)
{
    /* 1. Создаем RAW-сокет для перехвата ВСЕХ пакетов (ETH_P_ALL) */
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        perror("socket (нужен sudo!)");
        return -1;
    }

    /* 2. Получаем индекс интерфейса (enp0s3) */
    int if_index = if_nametoindex(ifname);
    if (if_index == 0) {
        fprintf(stderr, "Ошибка: интерфейс '%s' не найден\n", ifname);
        close(sock);
        return -1;
    }

    /* 3. Привязываем сокет к интерфейсу */
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = if_index;

    if (bind(sock, (struct sockaddr*)&sll, sizeof(sll)) < 0) {
        perror("bind");
        close(sock);
        return -1;
    }

    return sock;
}