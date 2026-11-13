/* ============================================================
 * main.c 
 *
 *   1. Парсинг argv: --clean <name> или <name>.
 *   2. Блокировка SIGINT ДО создания потоков (все унаследуют маску,
 *      сигнал достанется только sigwait в sig_thread).
 *   3. Определение роли через creator_run():
 *        вернул 1 -> я был создателем, всё сделано, выходим;
 *        вернул 0 -> очередь уже есть, идём в client_run().
 * ============================================================ */
#define _POSIX_C_SOURCE 200809L

#include "cleaner.h"
#include "client.h"
#include "creator.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    /* --- режим уборщика --- */
    if (argc == 3 && strcmp(argv[1], "--clean") == 0)
        return clean_queues(argv[2]) == 0 ? 0 : 1;

    /* --- обычный режим --- */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <name>\n       %s --clean <name>\n",
            argv[0], argv[0]);
        return 1;
    }

    /* Блокируем SIGINT до создания любых потоков: все потоки
     * унаследуют маску, и SIGINT достанется только sigwait. */
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    if (creator_run(argv[1]) == 0)
        return client_run(argv[1]) == 0 ? 0 : 1;

    return 0;
}