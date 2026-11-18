#include "subscriber.h"
#include "common.h"
#include <time.h>
#include <sys/select.h>

/* ============================================================
 * состояние подписчика
 * ============================================================ */

static volatile sig_atomic_t g_sig_count = 0;   /* 1=мягкий, 2=дочитать, 3=немедленно */
static volatile sig_atomic_t g_hb_flag = 0;   /* пришёл SIGUSR1 */

static void on_sigint(int s)
{
    (void)s;
    if (g_sig_count < 3) g_sig_count++;
    if (g_sig_count == 3) _exit(0);             /* стадия 3 — немедленно */
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

/* ============================================================
 * подсказка и ввод пользователя
 * ============================================================ */

static void print_help(void)
{
    printf("команды:\n"
        "  sub <топик>     — подписаться\n"
        "  unsub <топик>   — отписаться\n"
        "  list            — показать мои подписки\n"
        "  help            — эта подсказка\n");
    fflush(stdout);
}

static int send_user_cmd(int qid, pid_t me, struct msgbuf* m, int len)
{
    if (msgsnd(qid, m, (size_t)len, IPC_NOWAIT) == -1) {
        if (errno == EAGAIN) {
            fprintf(stderr, "подписчик: очередь переполнена\n");
            return 0;
        }
        if (errno == EIDRM) {
            fprintf(stderr, "подписчик: очередь удалена\n");
            return -1;
        }
        fprintf(stderr, "подписчик: msgsnd: %s\n", strerror(errno));
        return -1;
    }
    (void)me;
    return 0;
}

static void handle_user_input(int qid, pid_t me, const char* line)
{
    char cmd[32] = { 0 };
    char topic[TOPIC_MAX + 1] = { 0 };

    int n = sscanf(line, "%31s %64s", cmd, topic);
    if (n < 1) return;                         /* пустая строка */

    if (strcmp(cmd, "sub") == 0 || strcmp(cmd, "subscribe") == 0) {
        if (n < 2) { fprintf(stderr, "подписчик: укажи топик\n"); return; }
        struct msgbuf m;
        int len = msg_build_sub(&m, me, topic, 0);
        if (len > 0) send_user_cmd(qid, me, &m, len);
    }
    else if (strcmp(cmd, "unsub") == 0 || strcmp(cmd, "unsubscribe") == 0) {
        if (n < 2) { fprintf(stderr, "подписчик: укажи топик\n"); return; }
        struct msgbuf m;
        int len = msg_build_sub(&m, me, topic, 1);
        if (len > 0) send_user_cmd(qid, me, &m, len);
    }
    else if (strcmp(cmd, "list") == 0 || strcmp(cmd, "listsubs") == 0) {
        struct msgbuf m;
        int len = msg_build_listsubs(&m, me);
        if (len > 0) send_user_cmd(qid, me, &m, len);
    }
    else if (strcmp(cmd, "help") == 0) {
        print_help();
    }
    else {
        fprintf(stderr, "подписчик: неизвестная команда '%s' (help — подсказка)\n", cmd);
    }
}

/* ============================================================
 * обработка одного сообщения mtype=pid
 * ============================================================ */

static void print_topic_cb(const char* topic, size_t len, void* ctx)
{
    int* cnt = ctx;
    printf("  [%d] %.*s\n", (*cnt)++, (int)len, topic);
}

static void handle_delivery(parsed_t* p)
{
    switch (p->cmd) {
    case CMD_SEND:
        printf("получено: топик=%s от=%d: %.*s\n",
            p->arg, (int)p->pid,
            (int)p->payload_len,
            p->payload ? p->payload : "");
        fflush(stdout);
        break;
    case CMD_OK:
        printf("подтверждение: %s\n", p->arg);
        fflush(stdout);
        break;
    case CMD_ERR:
        fprintf(stderr, "ошибка от брокера (%s): %.*s\n",
            p->arg, (int)p->payload_len,
            p->payload ? p->payload : "");
        break;
    case CMD_SUBS: {
        printf("мои подписки:\n");
        int cnt = 0;
        int n = parse_subs_topics(p->payload, p->payload_len,
            print_topic_cb, &cnt);
        if (n < 0)
            fprintf(stderr, "подписчик: битый список подписок\n");
        else if (n == 0)
            printf("  (нет подписок)\n");
        fflush(stdout);
        break;
    }
    default:
        fprintf(stderr, "подписчик: неожиданная команда %s, игнор\n",
            cmd_name(p->cmd));
        break;
    }
}

/* 1 = обработали, 0 = пусто, -1 = EIDRM/ошибка */
static int recv_and_handle(int qid, pid_t me)
{
    struct msgbuf m;
    ssize_t n = msgrcv(qid, &m, sizeof(m.mtext), (long)me, IPC_NOWAIT);
    if (n == -1) {
        if (errno == ENOMSG) return 0;
        if (errno == EINTR)  return 0;
        if (errno == EIDRM) {
            fprintf(stderr, "подписчик: очередь удалена\n");
            return -1;
        }
        fprintf(stderr, "подписчик: msgrcv: %s\n", strerror(errno));
        return -1;
    }

    parsed_t p;
    int rc = msg_parse(m.mtext, (size_t)n, &p);
    if (rc != 0) {
        fprintf(stderr, "подписчик: плохое сообщение rc=%d, игнор\n", rc);
        return 1;
    }
    if (!cmd_to_subscriber(p.cmd)) {
        fprintf(stderr, "подписчик: неожиданная команда %s, игнор\n",
            cmd_name(p.cmd));
        return 1;
    }
    handle_delivery(&p);
    return 1;
}

/* ============================================================
 * main-логика подписчика
 * ============================================================ */

int run_subscriber(int argc, char** argv)
{
    int ntopics = argc - 2;
    char** topics = &argv[2];

    int qid = msgget(QKEY, 0);
    if (qid == -1) {
        if (errno == ENOENT)
            fprintf(stderr, "подписчик: брокер не запущен (очереди нет), выход\n");
        else
            fprintf(stderr, "подписчик: msgget: %s\n", strerror(errno));
        return 1;
    }

    struct sigaction sa = { 0 };
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    struct sigaction sh = { 0 };
    sh.sa_handler = on_sigusr1;
    sigemptyset(&sh.sa_mask);
    sh.sa_flags = 0;
    sigaction(SIGUSR1, &sh, NULL);

    pid_t me = getpid();

    /* --- подписка на темы из аргументов (если есть) --- */
    for (int i = 0; i < ntopics; i++) {
        struct msgbuf m;
        int n = msg_build_sub(&m, me, topics[i], 0);
        if (n <= 0) {
            fprintf(stderr, "подписчик: битый топик '%s', пропуск\n", topics[i]);
            continue;
        }
        if (msgsnd(qid, &m, (size_t)n, IPC_NOWAIT) == -1) {
            if (errno == EIDRM) {
                fprintf(stderr, "подписчик: очередь удалена при подписке\n");
                return 1;
            }
            fprintf(stderr, "подписчик: msgsnd(subscribe): %s\n",
                strerror(errno));
        }
    }

    printf("подписчик: pid=%d, топиков из аргументов: %d\n", (int)me, ntopics);
    print_help();

    time_t last_hb = time(NULL);
    int stdin_eof = 0;

    /* ---------- стадия 0: работа ---------- */
    while (g_sig_count == 0) {
        if (g_hb_flag) { g_hb_flag = 0; last_hb = time(NULL); }

        if (time(NULL) - last_hb >= ACTOR_DEAD_AFTER_SECONDS) {
            fprintf(stderr, "подписчик: брокер мёртв, шлём себе SIGINT\n");
            kill(getpid(), SIGINT);
            if (GRACE2_PAUSE_MS > 0) sleep_ms(GRACE2_PAUSE_MS);
            kill(getpid(), SIGINT);
            break;
        }

        /* 1) stdin — команды пользователя (только в стадии 0) */
        if (!stdin_eof) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(STDIN_FILENO, &rfds);
            struct timeval tv = { 0, 0 };
            if (select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv) > 0 &&
                FD_ISSET(STDIN_FILENO, &rfds)) {
                char line[256];
                if (fgets(line, sizeof(line), stdin)) {
                    handle_user_input(qid, me, line);
                }
                else if (feof(stdin)) {
                    stdin_eof = 1;         /* больше не читаем stdin */
                }
            }
        }

        /* 2) очередь mtype=pid */
        int r = recv_and_handle(qid, me);
        if (r == -1) break;
        if (r == 0) sleep_ms(POLL_MS);
    }

    /* ---------- стадия 1+2: мягкий режим → дочитать и выйти ---------- */
    if (g_sig_count >= 1) {
        fprintf(stderr, "подписчик: мягкое завершение, только чтение\n");
        while (g_sig_count < 3) {
            int r = recv_and_handle(qid, me);
            if (r == -1) break;
            if (r == 0) {
                if (g_sig_count >= 2) break;
                sleep_ms(POLL_MS);
            }
        }
        fprintf(stderr, "подписчик: выход\n");
    }

    return 0;
}