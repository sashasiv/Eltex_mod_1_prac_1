/* ============================================================
 *  debug.c — реализация отладочного дампа датаграмм.
 * ============================================================ */

#include "debug.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>

static int g_debug = 0;

void debug_set_enabled(int on) { g_debug = on ? 1 : 0; }
int  debug_enabled(void) { return g_debug; }

/* Печать hex+ASCII одной строки по 16 байт. */
static void dump_hex_ascii(const unsigned char* p, size_t len)
{
    for (size_t off = 0; off < len; off += 16) {
        printf("  %04zx  ", off);

        /* hex-часть */
        for (size_t i = 0; i < 16; i++) {
            if (off + i < len) printf("%02x ", p[off + i]);
            else               printf("   ");
            if (i == 7) printf(" ");
        }

        /* ASCII-часть */
        printf(" |");
        for (size_t i = 0; i < 16 && off + i < len; i++) {
            unsigned char c = p[off + i];
            putchar(isprint(c) ? c : '.');
        }
        printf("|\n");
    }
}

void debug_dump(const char* dir, const struct sockaddr_in* addr,
    const void* buf, size_t len)
{
    if (!g_debug) return;

    char ip[INET_ADDRSTRLEN] = "?";
    if (addr)
        inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));

    printf("[%s] %s:%u  len=%zu\n",
        dir, ip, addr ? ntohs(addr->sin_port) : 0, len);

    dump_hex_ascii((const unsigned char*)buf, len);
    fflush(stdout);
}