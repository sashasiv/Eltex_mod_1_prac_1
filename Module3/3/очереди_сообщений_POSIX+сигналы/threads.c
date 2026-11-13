/* ============================================================
 * threads.c 
 * ============================================================ */
#define _POSIX_C_SOURCE 200809L

#include "threads.h"
#include "common.h"
#include "message.h"
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

 /* ------------------------------------------------------------
  * 4.1. Ловим SIGINT через sigwait.
  *      1-е нажатие -> g_sigint_count = 1 (graceful)
  *      2-е и далее -> g_sigint_count = 2 (forced)
  * Поток бесконечный; процесс завершится по выходу из main.
  * ------------------------------------------------------------ */
void* sig_thread(void* arg) {
    sigset_t* set = arg;
    int sig;
    for (;;) {
        if (sigwait(set, &sig) != 0) continue;
        if (sig == SIGINT) {
            if (g_sigint_count == 0) g_sigint_count = 1;
            else                     g_sigint_count = 2;
        }
    }
    return NULL;
}

/* ------------------------------------------------------------
 * 4.2. sender: stdin -> mq_timedsend(PRIO_NORMAL).
 *
 * Цикл пока не (Ctrl+C) и не (стоп от партнёра).
 * mq_timedsend с TICK_MS — чтобы не залипнуть при полной очереди
 * и оставаться отзывчивым на флаги.
 *
 * При выходе из цикла шлём РОВНО ОДИН PRIO_STOP, затем ставим
 * g_sender_done (именно после фактической отправки — от этого
 * момента receiver отсчитывает grace-deadline).
 * ------------------------------------------------------------ */
void* sender_thread(void* arg) {
    (void)arg;
    char line[MSG_MAX];
    struct chat_msg msg;

    while (g_sigint_count == 0 && g_peer_stop == 0) {
        int r = wait_stdin(TICK_MS);
        if (r < 0) break;                            /* ошибка poll */
        if (r == 0) continue;                        /* таймаут — проверим флаги */

        if (!fgets(line, sizeof line, stdin)) break; /* EOF */
        strip_newline(line);
        if (line[0] == '\0') continue;               /* пустую не шлём */

        print_stdout("[you]  %s\n", line);
        memset(&msg, 0, sizeof msg);
        snprintf(msg.text, sizeof msg.text, "%s", line);

        /* mq_timedsend с TICK_MS: не залипаем на полной очереди */
        int sent = 0;
        while (!sent) {
            struct timespec ts = ms_to_ts(TICK_MS);
            if (mq_timedsend(g_q_out, (char*)&msg, sizeof msg,
                PRIO_NORMAL, &ts) == 0) {
                sent = 1;
            }
            else if (errno == ETIMEDOUT) {
                if (g_sigint_count >= 2 || g_peer_stop) goto send_stop;
                /* иначе — повторяем попытку */
            }
            else {
                perror("mq_timedsend");
                goto send_stop;
            }
        }
    }

send_stop:
    /* Ровно один стоп. Приоритет ниже обычного -> встанет в конец. */
    memset(&msg, 0, sizeof msg);
    for (;;) {
        struct timespec ts = ms_to_ts(TICK_MS);
        if (mq_timedsend(g_q_out, (char*)&msg, sizeof msg,
            PRIO_STOP, &ts) == 0)
            break;
        if (errno != ETIMEDOUT) { perror("mq_timedsend stop"); break; }
        if (g_sigint_count >= 2) break;              /* forced — не настаиваем */
        /* иначе — ещё одна попытка */
    }
    g_sender_done = 1;                               /* стоп фактически ушёл */
    return NULL;
}

/* ------------------------------------------------------------
 * 4.3. receiver: mq_timedreceive -> stdout.
 *
 * Выход из цикла:
 *   - пришёл PRIO_STOP от партнёра   -> g_peer_stop = 1
 *   - 2-е нажатие Ctrl+C (forced)    -> немедленно
 *   - grace-deadline истёк после того, как sender отправил стоп
 *     (отсчёт от g_sender_done, а не от первого Ctrl+C)
 *
 * После выхода, если не forced, — drain_queue (дочитать остатки).
 * ------------------------------------------------------------ */
void* receiver_thread(void* arg) {
    (void)arg;
    struct chat_msg msg;
    unsigned prio;
    int grace_active = 0;
    long long deadline = 0;

    for (;;) {
        struct timespec ts = ms_to_ts(TICK_MS);
        ssize_t n = mq_timedreceive(g_q_in, (char*)&msg, sizeof msg,
            &prio, &ts);
        if (n >= 0) {
            if (prio == PRIO_STOP) {
                g_peer_stop = 1;
                break;
            }
            /* НОВОЕ: ловим уведомление о подключении */
            if (prio == PRIO_CONNECT) {
                print_stdout("[system] peer connected\n");
                continue; /* переходим к следующей итерации, текст не печатаем */
            }
            print_stdout("[peer] %s\n", msg.text);
        }
        else if (errno != ETIMEDOUT) {
            perror("mq_timedreceive");
            break;
        }

        if (g_sigint_count >= 2) break;              /* forced */

        /* graceful: отсчёт от момента фактической отправки стопа */
        if (g_sender_done && !grace_active) {
            grace_active = 1;
            deadline = now_ms() + GRACE_TIMEOUT_MS;
        }
        if (grace_active && now_ms() >= deadline) break; /* таймаут ожидания */
    }

    if (g_sigint_count < 2) drain_queue(g_q_in);
    return NULL;
}