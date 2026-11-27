#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

/* 1 — пора завершаться. Ставится из обработчика сигнала. */
extern volatile sig_atomic_t g_stop;

/* Повесить ловушки на SIGINT и SIGTERM.
 * Без SA_RESTART — чтобы блокирующие вызовы (recvfrom/poll) возвращали EINTR
 * и цикл мог увидеть g_stop. */
void install_signal_handlers(void);

#endif /* SIGNALS_H */