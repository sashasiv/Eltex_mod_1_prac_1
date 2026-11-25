/* ============================================================
 *  main.c — сценарий жизни клиента UDP broadcast-чата.
 *
 *  Последовательность:
 *    1. разбор аргументов (имя, broadcast-адрес)
 *    2. инициализация client_t
 *    3. сокет + bind + broadcast-адрес
 *    4. JOIN
 *    5. запуск потока приёма
 *    6. главный цикл: stdin -> MSG (broadcast)
 *    7. LEAVE + остановка потока + cleanup
 * ============================================================ */

#include "chat.h"
#include "net.h"
#include "protocol.h"
#include "receiver.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <locale.h>
#include <pthread.h>

 /* Флаг остановки.  */
static volatile sig_atomic_t g_stop = 0;

static void on_sigint(int sig)
{
    (void)sig;
    g_stop = 1;
}

int main(int argc, char** argv)
{
    setlocale(LC_ALL, "");

    if (getenv("CHAT_DEBUG") || (argc >= 2 && strcmp(argv[1], "-d") == 0)) {
        debug_set_enabled(1);

        /* сдвиг: */
        for (int i = 1; i < argc - 1; i++)
            argv[i] = argv[i + 1];
        argv[argc - 1] = NULL;
        argc--;
    }

    if (argc < 3) {
        fprintf(stderr, "Использование: %s [-d] <имя> <broadcast_ip>\n", argv[0]);
        return 1;
    }

    /* ---------- 1. Контекст клиента ---------- */
    client_t c;
    memset(&c, 0, sizeof(c));

    strncpy(c.my_name, argv[1], NAME_LEN - 1);
    c.my_name[NAME_LEN - 1] = '\0';

    c.my_id = (unsigned)((unsigned)time(NULL) ^ (unsigned)getpid());
    c.running = 1;

    if (pthread_mutex_init(&c.print_mtx, NULL) != 0) {
        perror("pthread_mutex_init");
        return 1;
    }

    /* ---------- 2. Сеть ---------- */
    c.sock = net_create_socket();
    if (c.sock < 0) goto fail_mutex;

    if (net_bind(c.sock, PORT) < 0)          goto fail_sock;
    if (net_make_bcast(argv[2], PORT, &c.bcast) < 0) goto fail_sock;

    /* ---------- 3. JOIN ---------- */
    char out[BUF_SIZE];
    int len = proto_make_join(out, sizeof(out), c.my_id, c.my_name);
    if (len < 0) {
        fprintf(stderr, "Не удалось сформировать JOIN\n");
        goto fail_sock;
    }
    net_send(c.sock, &c.bcast, out, (size_t)len);

    receiver_print(&c, "Вы вошли в чат как %s (id=%u). Введите 'exit' для выхода.\n",
        c.my_name, c.my_id);

    /* ---------- 4. Поток приёма ---------- */
    pthread_t tid;
    if (pthread_create(&tid, NULL, receiver_thread, &c) != 0) {
        perror("pthread_create");
        goto fail_sock;
    }

    /* ---------- 5. SIGINT ---------- */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;                 /* без SA_RESTART: fgets вернёт EINTR */
    sigaction(SIGINT, &sa, NULL);

    /* ---------- 6. Главный цикл: stdin -> broadcast ---------- */
    char line[BUF_SIZE];

    while (c.running && !g_stop && fgets(line, sizeof(line), stdin)) {
        /* убрать завершающий \n */
        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, "exit") == 0) break;
        if (line[0] == '\0') continue;

        len = proto_make_msg(out, sizeof(out), c.my_id, c.my_name, line);
        if (len < 0) {
            receiver_print(&c, "[!] сообщение слишком длинное, не отправлено\n");
            continue;
        }
        net_send(c.sock, &c.bcast, out, (size_t)len);
    }

    /* ---------- 7. LEAVE и завершение ---------- */
    len = proto_make_leave(out, sizeof(out), c.my_id, c.my_name);
    if (len > 0)
        net_send(c.sock, &c.bcast, out, (size_t)len);

    receiver_stop(&c);          /* running = 0 + shutdown(sock) */
    pthread_join(tid, NULL);    /* дождаться реального выхода потока */
    close(c.sock);
    pthread_mutex_destroy(&c.print_mtx);

    receiver_print(&c, "Вы вышли из чата.\n");
    return 0;

fail_sock:
    close(c.sock);
fail_mutex:
    pthread_mutex_destroy(&c.print_mtx);
    return 1;
}