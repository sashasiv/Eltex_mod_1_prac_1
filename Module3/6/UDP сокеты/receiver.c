/* ============================================================
 *  receiver.c — поток приёма и обработки входящих сообщений.
 *
 *  Логика:
 *    recvfrom -> proto_parse -> фильтр своих -> печать.
 *  Выход: c->running = 0 + shutdown(sock), чтобы разбудить recvfrom.
 * ============================================================ */

#include "receiver.h"
#include "protocol.h"
#include "debug.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

 /* ---------- Печать под мьютексом ---------- */

void receiver_print(client_t* c, const char* fmt, ...)
{
    va_list ap;
    pthread_mutex_lock(&c->print_mtx);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
    pthread_mutex_unlock(&c->print_mtx);
}

/* ---------- Поток приёма ---------- */

void* receiver_thread(void* arg)
{
    client_t* c = (client_t*)arg;
    char buf[BUF_SIZE];
    struct sockaddr_in from;

    while (c->running) {
        socklen_t fromlen = sizeof(from);   /* сбрасывать КАЖДЫЙ раз */

        ssize_t n = recvfrom(c->sock, buf, sizeof(buf), 0,
            (struct sockaddr*)&from, &fromlen);
        if (n < 0) {
            if (errno == EINTR) continue;      /* сигнал — не ошибка */
            if (!c->running) break;            /* shutdown() разбудил */
            perror("recvfrom");
            continue;
        }
        if (n == 0) continue;                  /* пустая датаграмма */

        debug_dump("RECV", &from, buf, (size_t)n);

        message_t m;
        if (proto_parse(buf, (size_t)n, &m) != 0) continue;  /* мусор */
        if (m.id == c->my_id) continue;                      /* свои — мимо */

        switch (m.type) {
        case MSG_JOIN:
            receiver_print(c, "[+] %s вошёл в сеть\n", m.name);
            break;
        case MSG_LEAVE:
            receiver_print(c, "[-] %s покинул сеть\n", m.name);
            break;
        case MSG_CHAT:
            receiver_print(c, "%s: %s\n", m.name, m.text);
            break;
        default:
            break;
        }
    }

    return NULL;
}

/* ---------- Остановка потока ---------- */

void receiver_stop(client_t* c)
{
    c->running = 0;
    /* shutdown разблокирует recvfrom с errno == EBADF/ENOTCONN,
       поток увидит !running и выйдет из цикла */
    shutdown(c->sock, SHUT_RDWR);
}