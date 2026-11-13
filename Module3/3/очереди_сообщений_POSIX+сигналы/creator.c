/* ============================================================
 * creator.c — режим создателя.
 * ============================================================ */
#define _POSIX_C_SOURCE 200809L

#include "creator.h"
#include "common.h"
#include "message.h"
#include "config.h"
#include "threads.h"

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int creator_run(const char* base) {
    char name1[NAME_MAX_LEN], name2[NAME_MAX_LEN];
    build_names(base, name1, name2, sizeof name1);

    struct mq_attr attr;
    memset(&attr, 0, sizeof attr);
    attr.mq_maxmsg = Q_MAXMSG;
    attr.mq_msgsize = sizeof(struct chat_msg);

    /* --- _1: атомарно создаём. EEXIST -> я не создатель. --- */
    mqd_t q1 = mq_open(name1, O_CREAT | O_EXCL | O_RDWR, Q_MODE, &attr);
    if (q1 == (mqd_t)-1) {
        if (errno == EEXIST) return 0;        /* очередь уже есть -> клиент */
        perror("mq_open(_1)");
        exit(1);
    }

    /* --- _2: тоже создаём. Если EEXIST — аномалия (создатель упал
     *     между шагами): прибираем _1 за собой и выходим. --- */
    mqd_t q2 = mq_open(name2, O_CREAT | O_EXCL | O_RDWR, Q_MODE, &attr);
    if (q2 == (mqd_t)-1) {
        perror("mq_open(_2)");
        mq_close(q1);
        mq_unlink(name1);
        exit(1);
    }

    /* создатель: принимает из _1, отправляет в _2 */
    g_q_in = q1;
    g_q_out = q2;

    /* SIGINT уже заблокирован в main до pthread_create —
     * все потоки унаследовали маску, сигнал достанется sigwait. */
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);

    pthread_t t_sig, t_snd, t_rcv;
    pthread_create(&t_sig, NULL, sig_thread, &set);
    pthread_create(&t_snd, NULL, sender_thread, NULL);
    pthread_create(&t_rcv, NULL, receiver_thread, NULL);

    pthread_join(t_snd, NULL);
    pthread_join(t_rcv, NULL);
    /* sig_thread бесконечный — не join-им; умрёт вместе с процессом */

    print_stdout("[system] %s\n",
        g_sigint_count >= 2 ? "forced shutdown" :
        g_peer_stop ? "peer stopped" :
        "timeout waiting for peer");

    /* --- уборка: close обеих, затем unlink обоих имён --- */
    mq_close(q1);
    mq_close(q2);
    mq_unlink(name1);
    mq_unlink(name2);
    return 1;
}