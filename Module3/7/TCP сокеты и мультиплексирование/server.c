/* ============================================================
 *  server.c — TCP-сервер для чата с мультиплексированием (poll).
 * ============================================================ */

#include "chat.h"
#include "net.h"
#include "protocol.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_CLIENTS 100

int main(int argc, char** argv)
{
    // 1. Проверяем флаг -d
    int argi = 1;
    if (argi < argc && strcmp(argv[argi], "-d") == 0) {
        debug_set_enabled(1);
        argi++;
    }

    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    if (listen(listen_sock, 10) < 0) {
        perror("listen"); return 1;
    }

    printf("Сервер запущен на порту %d. Ожидание клиентов...\n", PORT);

    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;

    fds[0].fd = listen_sock;
    fds[0].events = POLLIN;

    while (1) {
        // Ждем событий на любом из сокетов
        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll"); break;
        }

        // 1. Новое подключение
        if (fds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &client_len);

            if (client_sock >= 0 && nfds < MAX_CLIENTS + 1) {
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
                printf("Новый клиент: %s:%d (fd=%d)\n", ip, ntohs(client_addr.sin_port), client_sock);

                fds[nfds].fd = client_sock;
                fds[nfds].events = POLLIN;
                nfds++;
            }
            else if (client_sock >= 0) {
                printf("Отклонено: достигнут лимит клиентов.\n");
                close(client_sock);
            }
        }

        // 2. Данные от существующих клиентов
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
                uint32_t net_len;

                // Читаем 4 байта длины
                if (tcp_recv_exact(fds[i].fd, (char*)&net_len, 4) < 0) {
                    printf("Клиент fd=%d отключился.\n", fds[i].fd);
                    close(fds[i].fd);
                    // Удаляем из массива (переставляем последний на место удаленного)
                    fds[i] = fds[nfds - 1];
                    nfds--; i--;
                    continue;
                }

                uint32_t msg_len = ntohl(net_len);
                if (msg_len == 0 || msg_len > BUF_SIZE * 2) {
                    printf("Ошибка: неверная длина сообщения от fd=%d (%u)\n", fds[i].fd, msg_len);
                    close(fds[i].fd);
                    fds[i] = fds[nfds - 1];
                    nfds--; i--;
                    continue;
                }

                // Читаем само сообщение
                char* msg = malloc(msg_len);
                if (tcp_recv_exact(fds[i].fd, msg, msg_len) < 0) {
                    printf("Ошибка чтения тела от fd=%d\n", fds[i].fd);
                    free(msg);
                    close(fds[i].fd);
                    fds[i] = fds[nfds - 1];
                    nfds--; i--;
                    continue;
                }

                // Пересылаем всем остальным
                for (int j = 1; j < nfds; j++) {
                    if (j != i) { // не отправляем отправителю
                        if (tcp_send_packet(fds[j].fd, msg, msg_len) < 0) {
                            // Если не смогли отправить, клиент отвалился
                            // (обработка ошибки отправки опущена для упрощения,
                            //  в реальном коде нужно закрыть и удалить)
                        }
                    }
                }
                free(msg);
            }
        }
    }

    close(listen_sock);
    return 0;
}