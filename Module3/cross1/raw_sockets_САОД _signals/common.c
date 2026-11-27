#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

void die(const char* msg)
{
    fprintf(stderr, "error: %s: %s\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

void die_usage(const char* prog)
{
    fprintf(stderr,
        "usage:\n"
        "  %s [-s <server_ip>] [-p <server_port>] [-c <client_port>]\n"
        "  (server: -s не нужен; client: -c обязателен или дефолт)\n",
        prog);
    exit(2);
}

int parse_ipv4(const char* s, uint32_t* out)
{
    struct in_addr a;
    if (inet_pton(AF_INET, s, &a) != 1)
        return -1;
    *out = ntohl(a.s_addr);
    return 0;
}

int parse_port(const char* s, uint16_t* out)
{
    char* end = NULL;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno || end == s || *end != '\0')
        return -1;
    if (v < 1 || v > 65535)
        return -1;
    *out = (uint16_t)v;
    return 0;
}

/* Классический трюк: UDP-сокет + connect() к dst — ядро выберет
 * локальный адрес интерфейса, через который пойдёт трафик.
 * Для 127.0.0.1 вернёт 127.0.0.1. */
int get_local_ip(uint32_t dst_ip, uint32_t* out_ip)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return -1;

    struct sockaddr_in dst = {
        .sin_family = AF_INET,
        .sin_port = htons(1),                 /* неважно, connect не шлёт */
        .sin_addr = { .s_addr = htonl(dst_ip) },
    };
    if (connect(fd, (struct sockaddr*)&dst, sizeof(dst)) < 0) {
        close(fd);
        return -1;
    }

    struct sockaddr_in local;
    socklen_t len = sizeof(local);
    if (getsockname(fd, (struct sockaddr*)&local, &len) < 0) {
        close(fd);
        return -1;
    }
    close(fd);

    *out_ip = ntohl(local.sin_addr.s_addr);
    return 0;
}