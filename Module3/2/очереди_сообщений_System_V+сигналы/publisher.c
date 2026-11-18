#include "publisher.h"
#include "common.h"
#include <time.h>

/* ============================================================
 * состояние издателя
 * ============================================================ */

static volatile sig_atomic_t g_sig_count = 0;   /* 1=мягкий, 2=дочитать, 3=немедленно */
static volatile sig_atomic_t g_hb_flag = 0;   /* пришёл SIGUSR1 */

static void on_sigint(int s)
{
    (void)s;
    if (g_sig_count < 3) g_sig_count++;
    if (g_sig_count == 3) _exit(0);
}

static void on_sigusr1(int s)
{
    (void)s;
    g_hb_flag = 1;
}

/* ============================================================
 * утилиты
 * ============================================================ */

static void sleep_ms(int ms)
{
    struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

static long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long)ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

static int make_payload(char* buf, size_t cap, unsigned counter)
{
    int n = snprintf(buf, cap, "сообщение #%u, время %ld", counter, (long)time(NULL));
    if (n < 0 || (size_t)n >= cap) return -1;
    return n;
}

/* ============================================================
 * отправка одного send
 * ============================================================ */

static int publish_once(int qid, const char* topic, unsigned* counter)
{
    char payload[PUBLISHER_PAYLOAD_MAX];
    int plen = make_payload(payload, sizeof(payload), *counter);
    if (plen < 0) return -1;

    struct msgbuf m;
    int n = msg_build_send(&m, getpid(), topic, payload, (size_t)plen);
    if (n <= 0) {
        fprintf(stderr, "издатель: ошибка сборки send\n");
        return -1;
    }

    if (msgsnd(qid, &m, (size_t)n, IPC_NOWAIT) == -1) {
        if (errno == EAGAIN) {
            fprintf(stderr, "издатель: очередь переполнена, пропуск\n");
            return 0;                       /* не фатально */
        }
        if (errno == EIDRM) {
            fprintf(stderr, "издатель: очередь удалена\n");
            return -1;
        }
        fprintf(stderr, "издатель: msgsnd: %s\n", strerror(errno));
        return -1;
    }
    (*counter)++;
    return 0;
}

/* ============================================================
 * main-логика издателя
 * ============================================================ */

int run_publisher(int argc, char** argv)
{
    (void)argc;
    const char* topic = argv[2];

    int qid = msgget(QKEY, 0);
    if (qid == -1) {
        if (errno == ENOENT)
            fprintf(stderr, "издатель: брокер не запущен (очереди нет), выход\n");
        else
            fprintf(stderr, "издатель: msgget: %s\n", strerror(errno));
        return 1;
    }

    struct sigaction sa = { 0 };
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;                        /* без SA_RESTART */
    sigaction(SIGINT, &sa, NULL);

    struct sigaction sh = { 0 };
    sh.sa_handler = on_sigusr1;
    sigemptyset(&sh.sa_mask);
    sh.sa_flags = 0;
    sigaction(SIGUSR1, &sh, NULL);

    time_t last_hb = time(NULL);
    unsigned counter = 0;
    long last_send = now_ms();

    /* ---------- стадия 0: работа ---------- */
    while (g_sig_count == 0) {
        if (g_hb_flag) { g_hb_flag = 0; last_hb = time(NULL); }

        /* heartbeat-таймер: брокер мёртв? */
        if (time(NULL) - last_hb >= ACTOR_DEAD_AFTER_SECONDS) {
            fprintf(stderr, "издатель: брокер мёртв, завершаем работу\n");
            kill(getpid(), SIGINT);
            if (GRACE2_PAUSE_MS > 0) sleep_ms(GRACE2_PAUSE_MS);
            kill(getpid(), SIGINT);
            break;
        }

        long now = now_ms();
        if (now - last_send >= PUBLISHER_INTERVAL_MS) {
            if (publish_once(qid, topic, &counter) != 0) break;
            last_send = now;
        }

        sleep_ms(POLL_MS);
    }

    /* ---------- стадия 1: мягкий режим ---------- */
    if (g_sig_count == 1) {
        fprintf(stderr, "издатель: мягкое завершение, отправка остановлена\n");
        /* ничего не шлём; ждём уровень 2 или 3 */
        while (g_sig_count == 1) sleep_ms(POLL_MS);
    }

    /* ---------- стадия 2/3: выход ---------- */
    fprintf(stderr, "издатель: выход\n");
    return 0;
}