/* ============================================================
 * common.h — общие флаги, состояние очередей и хелперы.
 * ============================================================ */
#ifndef COMMON_H
#define COMMON_H

#include <mqueue.h>
#include <signal.h>
#include <stddef.h>
#include <time.h>

 /* --- флаги завершения (один писатель на флаг — см. комментарии) --- */
extern volatile sig_atomic_t g_sigint_count;  /* пишет только sig_thread      */
extern volatile sig_atomic_t g_peer_stop;     /* пишет только receiver_thread */
extern volatile sig_atomic_t g_sender_done;   /* пишет только sender_thread   */

/* --- текущие очереди: откуда принимаем / куда отправляем ---
 *   создатель: g_q_in  = _1, g_q_out = _2
 *   клиент   : g_q_in  = _2, g_q_out = _1
 */
extern mqd_t g_q_in;
extern mqd_t g_q_out;

/* --- хелперы --- */
void        print_stdout(const char* fmt, ...);   /* printf под мьютексом */
long long   now_ms(void);                         /* CLOCK_MONOTONIC, мс  */
struct timespec ms_to_ts(long ms);
void        strip_newline(char* s);
int         wait_stdin(int timeout_ms);           /* poll(stdin), 1/0/-1  */
void        drain_queue(mqd_t q);                 /* неблокирующе дочитать */
void        build_names(const char* base,
    char* n1, char* n2, size_t cap);

#endif /* COMMON_H */