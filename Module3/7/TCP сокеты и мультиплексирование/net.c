/* ============================================================
 *  net.c — реализация сетевого слоя (UDP, broadcast).
 * ============================================================ */

#include "net.h"
#include "debug.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdint.h>

int tcp_send_packet(int sock, const char* buf, size_t len)
{
    /* 1. Отправляем длину (4 байта в сетевом порядке) */
    uint32_t net_len = htonl((uint32_t)len);
    ssize_t sent = send(sock, &net_len, sizeof(net_len), 0);
    if (sent != sizeof(net_len)) {
        perror("send length");
        return -1;
    }

    /* 2. Отправляем само сообщение */
    size_t total_sent = 0;
    while (total_sent < len) {
        sent = send(sock, buf + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            perror("send body");
            return -1;
        }
        total_sent += sent;
    }

    /* === ДЕБАГ: логируем отправку с реальным IP === */
    if (debug_enabled()) {
        struct sockaddr_in peer_addr;
        socklen_t addr_len = sizeof(peer_addr);
        if (getpeername(sock, (struct sockaddr*)&peer_addr, &addr_len) == 0) {
            debug_dump("TCP SEND", &peer_addr, buf, len);
        }
    }

    return 0;
}

int tcp_recv_exact(int sock, char* buf, size_t len)
{
    size_t total_recv = 0;
    while (total_recv < len) {
        ssize_t n = recv(sock, buf + total_recv, len - total_recv, 0);
        if (n < 0) {
            if (errno == EINTR) continue; /* Прервано сигналом — читаем дальше */
            perror("recv");
            return -1;
        }
        if (n == 0) {
            /* Соединение закрыто другой стороной */
            return -1;
        }
        total_recv += n;
    }

    /* === ДЕБАГ: логируем прием с реальным IP === */
    if (debug_enabled()) {
        struct sockaddr_in peer_addr;
        socklen_t addr_len = sizeof(peer_addr);
        if (getpeername(sock, (struct sockaddr*)&peer_addr, &addr_len) == 0) {
            debug_dump("TCP RECV", &peer_addr, buf, len);
        }
    }

    return 0;
}