/* ============================================================
 * client.c — блок 6: режим клиента.
 * ============================================================ */
#define _POSIX_C_SOURCE 200809L

#include "client.h"
#include "common.h"
#include "config.h"
#include "threads.h"
#include "message.h"

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

int client_run(const char* base) {
    char name1[NAME_MAX_LEN], name2[NAME_MAX_LEN];
    build_names(base, name1, name2, sizeof name1);

    /* клиент: отправляет в _1, принимает из _2.
     * O_RDWR без O_CREAT — очереди уже должны существовать. */
    mqd_t q1 = mq_open(name1, O_RDWR);
    if (q1 == (mqd_t)-1) {
        perror("mq_open(_1)");
        return -1;
    }
    mqd_t q2 = mq_open(name2, O_RDWR);
    if (q2 == (mqd_t)-1) {
        perror("mq_open(_2)");
        mq_close(q1);
        return -1;
    }

    g_q_out = q1;   /* в _1 */
    g_q_in = q2;   /* из _2 */

    /* уведомляем создателя о подключении --- */
    struct chat_msg hello;
    memset(&hello, 0, sizeof hello);
    /* Отправляем с высоким приоритетом, чтобы создатель сразу увидел */
    if (mq_send(g_q_out, (char*)&hello, sizeof hello, PRIO_CONNECT) == -1) {
        perror("mq_send connect");
    }

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);

    pthread_t t_sig, t_snd, t_rcv;
    pthread_create(&t_sig, NULL, sig_thread, &set);
    pthread_create(&t_snd, NULL, sender_thread, NULL);
    pthread_create(&t_rcv, NULL, receiver_thread, NULL);

    pthread_join(t_snd, NULL);
    pthread_join(t_rcv, NULL);

    print_stdout("[system] %s\n",
        g_sigint_count >= 2 ? "forced shutdown" :
        g_peer_stop ? "peer stopped" :
        "timeout waiting for peer");

    mq_close(q1);
    mq_close(q2);
    /* unlink НЕ делаем — по заданию это делает создатель */
    return 0;
}