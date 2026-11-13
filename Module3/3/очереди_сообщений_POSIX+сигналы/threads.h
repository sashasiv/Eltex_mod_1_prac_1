/* ============================================================
 * threads.h — три потока программы.
 *
 *   sig_thread      — sigwait(SIGINT): 1-е нажатие -> graceful,
 *                     2-е -> forced. Пишет g_sigint_count.
 *   sender_thread   — stdin -> mq_timedsend(PRIO_NORMAL). При выходе
 *                     шлёт ровно один PRIO_STOP и ставит g_sender_done.
 *   receiver_thread — mq_timedreceive -> stdout. PRIO_STOP -> g_peer_stop.
 *                     Grace-deadline отсчитывается от g_sender_done.
 * ============================================================ */
#ifndef THREADS_H
#define THREADS_H

#include <signal.h>

void* sig_thread(void* arg);        /* arg = sigset_t* с SIGINT            */
void* sender_thread(void* arg);     /* arg не используется                  */
void* receiver_thread(void* arg);   /* arg не используется                  */

#endif /* THREADS_H */