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
#include <sys/socket.h>

 /* ---------- Создание сокета ---------- */
int net_create_socket(void)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    int yes = 1;

    /* Без SO_BROADCAST sendto() на broadcast-адрес вернёт EACCES */
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0) {
        perror("setsockopt SO_BROADCAST");
        close(sock);
        return -1;
    }

    /* Позволяет нескольким клиентам слушать один порт на одной машине */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        close(sock);
        return -1;
    }

#ifdef SO_REUSEPORT
    /* Чтобы два процесса реально делили один UDP-порт (для теста на одной машине) */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) < 0) {
        perror("setsockopt SO_REUSEPORT");
        close(sock);
        return -1;
    }
#endif

    return sock;
}

/* ---------- Привязка к порту ---------- */
int net_bind(int sock, int port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   /* слушать все интерфейсы */

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }
    return 0;
}

/* ---------- Формирование broadcast-адреса ---------- */
int net_make_bcast(const char* ip, int port, struct sockaddr_in* out)
{
    memset(out, 0, sizeof(*out));
    out->sin_family = AF_INET;
    out->sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, ip, &out->sin_addr) != 1) {
        fprintf(stderr, "net_make_bcast: неверный IP '%s'\n", ip);
        return -1;
    }
    return 0;
}

/* ---------- Отправка ---------- */
int net_send(int sock, const struct sockaddr_in* dst,
    const char* buf, size_t len)
{
    ssize_t n = sendto(sock, buf, len, 0,
        (const struct sockaddr*)dst, sizeof(*dst));
    if (n < 0) { perror("sendto"); return -1; }
    debug_dump("SEND", dst, buf, (size_t)n);
    return (int)n;
}