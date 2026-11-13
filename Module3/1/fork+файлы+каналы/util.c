// util.c
#include "util.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* ---------- небуферизованный I/O ---------- */

ssize_t write_full(int fd, const void* buf, size_t n) {
    const char* p = buf;
    size_t left = n;

    while (left > 0) {
        ssize_t w = write(fd, p, left);
        if (w < 0) {
            if (errno == EINTR) continue;   /* сигнал прервал — повторяем */
            return -1;
        }
        if (w == 0) break;                  /* писать больше некуда */
        p += w;
        left -= (size_t)w;
    }
    return (ssize_t)(n - left);
}

ssize_t read_full(int fd, void* buf, size_t n) {
    char* p = buf;
    size_t left = n;

    while (left > 0) {
        ssize_t r = read(fd, p, left);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) break;                  /* EOF */
        p += r;
        left -= (size_t)r;
    }
    return (ssize_t)(n - left);
}

/* ---------- сообщения протокола ---------- */

int send_msg(int fd, const struct msg* m) {
    ssize_t w = write_full(fd, m, sizeof(*m));
    return (w == (ssize_t)sizeof(*m)) ? 0 : -1;
}

int recv_msg(int fd, struct msg* m) {
    ssize_t r = read_full(fd, m, sizeof(*m));
    if (r < 0)                         return -1;   /* ошибка read */
    if (r == 0)                        return  0;   /* чистый EOF */
    if (r != (ssize_t)sizeof(*m))      return -2;   /* обрезанное сообщение */
    return 1;
}

/* ---------- логирование ---------- */

void log_msg(const char* role, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[%s pid=%d] ", role, (int)getpid());
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}