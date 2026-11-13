/* ============================================================
 * common.c — определения глобалов и реализации хелперов.
 * ============================================================ */
#define _POSIX_C_SOURCE 200809L

#include "common.h"
#include "message.h"

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

 /* --- единственные экземпляры глобалов --- */
volatile sig_atomic_t g_sigint_count = 0;
volatile sig_atomic_t g_peer_stop = 0;
volatile sig_atomic_t g_sender_done = 0;

mqd_t g_q_in = (mqd_t)-1;
mqd_t g_q_out = (mqd_t)-1;

/* --- мьютекс на stdout (sender и receiver пишут параллельно) --- */
static pthread_mutex_t g_out_mtx = PTHREAD_MUTEX_INITIALIZER;

void print_stdout(const char* fmt, ...) {
    va_list ap;
    pthread_mutex_lock(&g_out_mtx);
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fflush(stdout);
    pthread_mutex_unlock(&g_out_mtx);
}

long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

struct timespec ms_to_ts(long ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    return ts;
}

void strip_newline(char* s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r'))
        s[--n] = '\0';
}

/* 1 = stdin готов, 0 = таймаут, -1 = ошибка poll */
int wait_stdin(int timeout_ms) {
    struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN };
    int rc = poll(&pfd, 1, timeout_ms);
    if (rc <= 0) return rc;
    return (pfd.revents & (POLLIN | POLLHUP)) ? 1 : 0;
}

/* Неблокирующе дочитать очередь. PRIO_STOP игнорируем — он уже обработан. */
void drain_queue(mqd_t q) {
    struct chat_msg msg;
    unsigned prio;
    struct timespec zero = { 0, 0 };
    for (;;) {
        ssize_t n = mq_timedreceive(q, (char*)&msg, sizeof msg, &prio, &zero);
        if (n < 0) break;                 /* EAGAIN — пусто (или ошибка) */
        if (prio == PRIO_STOP) continue;
        print_stdout("[peer] %s\n", msg.text);
    }
}

void build_names(const char* base, char* n1, char* n2, size_t cap) {
    while (*base == '/') base++;          /* пропускаем ведущие слэши */
    snprintf(n1, cap, "/%s_1", base);
    snprintf(n2, cap, "/%s_2", base);
}