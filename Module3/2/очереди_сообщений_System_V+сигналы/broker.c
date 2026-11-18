#include "broker.h"
#include "common.h"
#include <time.h>

/* ============================================================
 * состояние брокера
 * ============================================================ */

typedef struct {
    pid_t pid;
    char  topics[MAX_TOPICS_PER_SUB][TOPIC_MAX + 1];
    int   ntopics;
} sub_entry_t;

typedef struct {
    sub_entry_t subs[MAX_SUBS];
    int         nsubs;
    pid_t       pubs[MAX_PUBS];
    int         npubs;
} broker_state_t;

static volatile sig_atomic_t g_sig_count = 0;   /* 1=мягкий, 2=жёсткий */

static void on_sigint(int s)
{
    (void)s;
    if (g_sig_count < 2) g_sig_count++;
}

/* ============================================================
 * утилиты
 * ============================================================ */

static void sleep_ms(int ms)
{
    struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

static void broadcast_sigint(broker_state_t* st)
{
    for (int i = 0; i < st->nsubs; i++) kill(st->subs[i].pid, SIGINT);
    for (int i = 0; i < st->npubs; i++) kill(st->pubs[i], SIGINT);
}

static void broadcast_heartbeat(broker_state_t* st)
{
    for (int i = 0; i < st->nsubs; i++) kill(st->subs[i].pid, SIGUSR1);
    for (int i = 0; i < st->npubs; i++) kill(st->pubs[i], SIGUSR1);
}

/* ============================================================
 * таблицы
 * ============================================================ */

static sub_entry_t* find_sub(broker_state_t* st, pid_t pid)
{
    for (int i = 0; i < st->nsubs; i++)
        if (st->subs[i].pid == pid) return &st->subs[i];
    return NULL;
}

static int sub_has_topic(const sub_entry_t* s, const char* topic)
{
    for (int i = 0; i < s->ntopics; i++)
        if (strcmp(s->topics[i], topic) == 0) return 1;
    return 0;
}

static int add_sub_topic(broker_state_t* st, pid_t pid, const char* topic)
{
    sub_entry_t* s = find_sub(st, pid);
    if (!s) {
        if (st->nsubs >= MAX_SUBS) return -1;
        s = &st->subs[st->nsubs++];
        s->pid = pid;
        s->ntopics = 0;
    }
    if (sub_has_topic(s, topic)) return 0;              /* идемпотентно */
    if (s->ntopics >= MAX_TOPICS_PER_SUB) return -1;
    strncpy(s->topics[s->ntopics], topic, TOPIC_MAX);
    s->topics[s->ntopics][TOPIC_MAX] = '\0';
    s->ntopics++;
    return 0;
}

static int del_sub_topic(broker_state_t* st, pid_t pid, const char* topic)
{
    sub_entry_t* s = find_sub(st, pid);
    if (!s) return -1;
    for (int i = 0; i < s->ntopics; i++) {
        if (strcmp(s->topics[i], topic) == 0) {
            memmove(&s->topics[i], &s->topics[i + 1],
                (s->ntopics - i - 1) * sizeof(s->topics[0]));
            s->ntopics--;
            if (s->ntopics == 0) {
                int idx = (int)(s - st->subs);
                memmove(&st->subs[idx], &st->subs[idx + 1],
                    (st->nsubs - idx - 1) * sizeof(st->subs[0]));
                st->nsubs--;
            }
            return 0;
        }
    }
    return -1;
}

static void add_pub(broker_state_t* st, pid_t pid)
{
    for (int i = 0; i < st->npubs; i++)
        if (st->pubs[i] == pid) return;
    if (st->npubs < MAX_PUBS) st->pubs[st->npubs++] = pid;
}

/* ============================================================
 * отправка
 * ============================================================ */

static int send_msg(int qid, struct msgbuf* m, int len)
{
    if (msgsnd(qid, m, (size_t)len, IPC_NOWAIT) == -1) {
        if (errno == EAGAIN) {
            if (BROKER_DEBUG)
                fprintf(stderr, "брокер: очередь переполнена, пропуск\n");
            return -1;
        }
        if (errno == EIDRM) {
            if (BROKER_DEBUG)
                fprintf(stderr, "брокер: очередь удалена\n");
            g_sig_count = 2;
            return -1;
        }
        fprintf(stderr, "брокер: msgsnd: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

/* ============================================================
 * обработчики команд
 * ============================================================ */

static void handle_send(int qid, broker_state_t* st, pid_t from_pid,
    const char* topic, const char* raw, size_t rawlen)
{
    add_pub(st, from_pid);

    struct msgbuf out;
    memcpy(out.mtext, raw, rawlen);        /* копия исходного mtext */

    for (int i = 0; i < st->nsubs; i++) {
        if (!sub_has_topic(&st->subs[i], topic)) continue;
        out.mtype = st->subs[i].pid;
        send_msg(qid, &out, (int)rawlen);
    }
}

static void handle_subscribe(int qid, broker_state_t* st, pid_t pid,
    const char* topic)
{
    if (add_sub_topic(st, pid, topic) != 0) {
        struct msgbuf e;
        int n = msg_build_err(&e, pid, "subscribe", "слишком много тем");
        if (n > 0) send_msg(qid, &e, n);
        return;
    }
    struct msgbuf ok;
    int n = msg_build_ok(&ok, pid, "subscribe");
    if (n > 0) send_msg(qid, &ok, n);
}

static void handle_unsubscribe(int qid, broker_state_t* st, pid_t pid,
    const char* topic)
{
    if (del_sub_topic(st, pid, topic) != 0) {
        struct msgbuf e;
        int n = msg_build_err(&e, pid, "unsubscribe", "нет такой подписки");
        if (n > 0) send_msg(qid, &e, n);
        return;
    }
    struct msgbuf ok;
    int n = msg_build_ok(&ok, pid, "unsubscribe");
    if (n > 0) send_msg(qid, &ok, n);
}

static void handle_listsubs(int qid, broker_state_t* st, pid_t pid)
{
    sub_entry_t* s = find_sub(st, pid);
    struct msgbuf sm;
    int n = msg_build_subs(&sm, pid,
        s ? s->topics : NULL,
        s ? s->ntopics : 0);
    if (n > 0) send_msg(qid, &sm, n);
}

/* ============================================================
 * приём одного сообщения
 * ============================================================ */

 /* 1 = обработали, 0 = пусто (ENOMSG), -1 = EIDRM/ошибка */
static int recv_and_handle(int qid, broker_state_t* st)
{
    struct msgbuf m;
    ssize_t n = msgrcv(qid, &m, sizeof(m.mtext), MTYPE_TO_BROKER,
        IPC_NOWAIT);
    if (n == -1) {
        if (errno == ENOMSG) return 0;
        if (errno == EINTR)  return 0;
        if (errno == EIDRM) {
            if (BROKER_DEBUG)
                fprintf(stderr, "брокер: очередь удалена\n");
            g_sig_count = 2;
            return -1;
        }
        fprintf(stderr, "брокер: msgrcv: %s\n", strerror(errno));
        return -1;
    }

    parsed_t p;
    int rc = msg_parse(m.mtext, (size_t)n, &p);
    if (rc != 0) {
        if (BROKER_DEBUG)
            fprintf(stderr, "брокер: плохое сообщение rc=%d, игнор\n", rc);
        return 1;
    }
    if (!cmd_to_broker(p.cmd)) {
        if (BROKER_DEBUG)
            fprintf(stderr, "брокер: неожиданная команда %s, игнор\n",
                cmd_name(p.cmd));
        return 1;
    }

    switch (p.cmd) {
    case CMD_SEND:
        handle_send(qid, st, p.pid, p.arg, m.mtext, (size_t)n);
        break;
    case CMD_SUBSCRIBE:
        handle_subscribe(qid, st, p.pid, p.arg);
        break;
    case CMD_UNSUBSCRIBE:
        handle_unsubscribe(qid, st, p.pid, p.arg);
        break;
    case CMD_LISTSUBS:
        handle_listsubs(qid, st, p.pid);
        break;
    default:
        break;
    }
    return 1;
}

/* ============================================================
 * main
 * ============================================================ */

int run_broker(int argc, char** argv)
{
    (void)argc; (void)argv;

    int qid = msgget(QKEY, IPC_CREAT | IPC_EXCL | 0666);
    if (qid == -1) {
        if (errno == EEXIST)
            fprintf(stderr, "брокер: очередь уже существует, выход\n");
        else
            fprintf(stderr, "брокер: msgget: %s\n", strerror(errno));
        return 1;
    }

    struct sigaction sa = { 0 };
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;                      /* без SA_RESTART */
    sigaction(SIGINT, &sa, NULL);

    broker_state_t st = { 0 };
    time_t last_hb = time(NULL);

    /* ---------- стадия 0: обычная работа ---------- */
    while (g_sig_count == 0) {
        int r = recv_and_handle(qid, &st);
        if (r == -1) break;

        /* heartbeat: раз в BROKER_HEARTBEAT_SECONDS */
        if (time(NULL) - last_hb >= BROKER_HEARTBEAT_SECONDS) {
            broadcast_heartbeat(&st);
            last_hb = time(NULL);
        }

        /* спим только если очередь по mtype=1 пуста */
        if (r == 0) sleep_ms(POLL_MS);
    }

    /* ---------- стадия 1: мягкий режим ---------- */
    if (g_sig_count >= 1) {
        if (BROKER_DEBUG) fprintf(stderr, "брокер: мягкое завершение\n");
        broadcast_sigint(&st);                  /* уровень 1: все замолкают */
        time_t t1 = time(NULL);
        int soft_ok = 0;

        while (g_sig_count < 2) {
            int r = recv_and_handle(qid, &st);
            if (r == -1) break;

            /* вся очередь пуста? (в мягком режиме никто не пишет) */
            struct msqid_ds ds;
            if (msgctl(qid, IPC_STAT, &ds) == 0 && ds.msg_qnum == 0) {
                soft_ok = 1;
                break;
            }

            if (time(NULL) - t1 > GRACE1_SECONDS) break;
            if (r == 0) sleep_ms(POLL_MS);
        }

        if (soft_ok) {
            if (GRACE2_PAUSE_MS > 0) sleep_ms(GRACE2_PAUSE_MS);
            if (BROKER_DEBUG) fprintf(stderr, "брокер: очередь пуста\n");
            broadcast_sigint(&st);              /* уровень 2 */
        }
        else {
            if (BROKER_DEBUG) fprintf(stderr, "брокер: жёсткое завершение\n");
            broadcast_sigint(&st);              /* уровень 2 */
            if (GRACE2_PAUSE_MS > 0) sleep_ms(GRACE2_PAUSE_MS);
            broadcast_sigint(&st);              /* уровень 3 */
        }
    }

    msgctl(qid, IPC_RMID, NULL);
    if (BROKER_DEBUG) fprintf(stderr, "брокер: очередь удалена, выход\n");
    return 0;
}