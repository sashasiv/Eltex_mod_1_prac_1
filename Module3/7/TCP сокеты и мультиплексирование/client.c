/* ============================================================
 *  client.c — TCP-клиент с мультиплексированием (select).
 * ============================================================ */

#include "chat.h"
#include "net.h"
#include "protocol.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char** argv)
{
    // 1. Проверяем флаг -d
    int argi = 1;
    if (argi < argc && strcmp(argv[argi], "-d") == 0) {
        debug_set_enabled(1);
        argi++;
    }

    // 2. Проверяем остальные аргументы (смещаем индекс)
    if (argc - argi < 2) {
        printf("Использование: %s [-d] <имя> <ip_сервера>\n", argv[0]);
        return 1;
    }

    const char* name = argv[argi];
    const char* server_ip = argv[argi + 1];

    unsigned my_id = (unsigned)(time(NULL) ^ getpid());

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, server_ip, &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect"); return 1;
    }

    printf("Подключено к серверу %s. Введите 'exit' для выхода.\n", server_ip);
    printf("Команда для отправки файла: /send <имя_файла>\n> ");

    // Отправляем JOIN
    char buf[BUF_SIZE * 2];
    int len = proto_make_join(buf, sizeof(buf), my_id, name);
    tcp_send_packet(sock, buf, len);

    fd_set readfds;
    char line[BUF_SIZE];
    char out_buf[BUF_SIZE * 2];

    /* --- Состояние для приема файлов --- */
    FILE* incoming_file = NULL;
    size_t file_received = 0;
    size_t file_expected = 0;
    int file_packets = 0;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);

        int max_fd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

        int ret = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (ret < 0) break;

        // 1. Данные от сервера
        if (FD_ISSET(sock, &readfds)) {
            uint32_t net_len;
            if (tcp_recv_exact(sock, (char*)&net_len, 4) < 0) {
                printf("\rСервер отключился.\n");
                break;
            }

            uint32_t msg_len = ntohl(net_len);
            char* msg = malloc(msg_len);

            if (tcp_recv_exact(sock, msg, msg_len) == 0) {

                // Если мы УЖЕ принимаем файл, любые данные от сервера — это кусок файла
                if (incoming_file) {
                    size_t to_write = msg_len;

                    // Защита, чтобы не записать лишнего
                    if (file_received + to_write > file_expected) {
                        to_write = file_expected - file_received;
                    }

                    fwrite(msg, 1, to_write, incoming_file);
                    file_received += to_write;
                    file_packets++;

                    int percent = (int)((file_received * 100) / file_expected);
                    printf("\r[ФАЙЛ] Прогресс: %zu/%zu байт (%d%%) | Пакетов: %d",
                        file_received, file_expected, percent, file_packets);
                    fflush(stdout);

                    if (file_received >= file_expected) {
                        fclose(incoming_file);
                        incoming_file = NULL;
                        printf("\r[ФАЙЛ] Прием завершен! Сохранен.                        \n> ");
                        fflush(stdout);
                    }
                }
                else {
                    // Мы НЕ в режиме приема файла, значит это должно быть обычное сообщение или заголовок
                    message_t m;
                    if (proto_parse(msg, msg_len, &m) == 0 && m.id != my_id) {
                        if (m.type == MSG_FILE) {
                            printf("\n[DEBUG] Пришел MSG_FILE! Имя: '%s', Размер: %zu\n", m.text, m.file_size);
                            fflush(stdout); // Принудительно выводим

                            incoming_file = fopen(m.text, "wb");
                            if (!incoming_file) {
                                printf("[DEBUG] ОШИБКА: не могу создать файл '%s'\n", m.text);
                                perror("fopen"); // Покажет системную ошибку (например, Permission denied)
                            }
                            else {
                                printf("[DEBUG] Файл успешно открыт для записи!\n");
                                file_received = 0;
                                file_expected = m.file_size;
                                file_packets = 0;
                                printf("\r[ФАЙЛ] %s отправляет '%s' (%zu байт). Принимаем...\n> ", m.name, m.text, m.file_size);
                                fflush(stdout);
                                if (file_expected == 0) {
                                    fclose(incoming_file);
                                    incoming_file = NULL;
                                    printf("\r[ФАЙЛ] Файл пуст. Прием завершен.                        \n> ");
                                }
                            }
                        }
                        else if (m.type == MSG_CHAT) { // <-- Только один раз!
                            printf("\r%s: %s\n> ", m.name, m.text);
                            fflush(stdout);
                        }
                        else if (m.type == MSG_JOIN) {
                            printf("\r[+] %s вошёл в сеть\n> ", m.name);
                            fflush(stdout);
                        }
                        else if (m.type == MSG_LEAVE) {
                            printf("\r[-] %s покинул сеть\n> ", m.name);
                            fflush(stdout);
                        }
                    }
                }
            }
            free(msg);
        }

        // 2. Ввод с клавиатуры
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (!fgets(line, sizeof(line), stdin)) break;
            line[strcspn(line, "\n")] = '\0';

            if (strcmp(line, "exit") == 0) break;
            if (line[0] == '\0') continue;

            if (strncmp(line, "/send ", 6) == 0) {
                const char* filename = line + 6;
                FILE* f = fopen(filename, "rb");
                if (!f) {
                    printf("Не удалось открыть файл %s\n> ", filename);
                    continue;
                }
                fseek(f, 0, SEEK_END);
                size_t fsize = ftell(f);
                fseek(f, 0, SEEK_SET);

                // Отправляем заголовок файла
                len = proto_make_file(out_buf, sizeof(out_buf), my_id, name, filename, fsize);
                tcp_send_packet(sock, out_buf, len);

                // Отправляем файл кусками
                char chunk[4096];
                size_t n;
                size_t sent_total = 0;

                // Отключаем дебаг на время отправки файла, чтобы не спамить
                int old_debug = debug_enabled();
                if (old_debug) debug_set_enabled(0);

                while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0) {
                    tcp_send_packet(sock, chunk, n);
                    sent_total += n;
                }

                if (old_debug) debug_set_enabled(1);

                fclose(f);
                printf("\r[ФАЙЛ] Отправлен %s (%zu байт)\n> ", filename, sent_total);
                fflush(stdout);
            }
            else {
                len = proto_make_msg(out_buf, sizeof(out_buf), my_id, name, line);
                tcp_send_packet(sock, out_buf, len);
                printf("> ");
                fflush(stdout);
            }
        }
    }

    // Отправляем LEAVE
    len = proto_make_leave(buf, sizeof(buf), my_id, name);
    tcp_send_packet(sock, buf, len);

    if (incoming_file) fclose(incoming_file);
    close(sock);
    printf("Вы вышли из чата.\n");
    return 0;
}