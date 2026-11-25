/* ============================================================
 *  main.c — Программа перехвата UDP-сегментов (Задание 8).
 *  Запуск: sudo ./sniffer <интерфейс> <фильтр>
 *  Фильтры: chat (5000/8080), dns (53), all (все UDP)
 * ============================================================ */

#include "net.h"
#include "sniff_parse.h"
#include "filter.h"
#include "sniff_output.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>

void print_usage(const char* prog) {
    printf("Использование: sudo %s <интерфейс> <фильтр>\n", prog);
    printf("  Интерфейс: enp0s3, eth0 и т.д. (смотрите через 'ip a')\n");
    printf("  Фильтры:\n");
    printf("    chat - UDP-чат из Задания 6 (порты 5000, 8080)\n");
    printf("    dns  - DNS-запросы (порт 53)\n");
    printf("    all  - Все UDP пакеты\n");
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char* ifname = argv[1];
    const char* filter_str = argv[2];
    int filter_type = 0;

    /* Определяем выбранный фильтр */
    if (strcmp(filter_str, "chat") == 0) filter_type = FILTER_CHAT;
    else if (strcmp(filter_str, "dns") == 0) filter_type = FILTER_DNS;
    else if (strcmp(filter_str, "all") == 0) filter_type = FILTER_ALL;
    else {
        fprintf(stderr, "Неизвестный фильтр: %s\n", filter_str);
        print_usage(argv[0]);
        return 1;
    }

    /* 1. Создаем RAW-сокет */
    int sock = raw_socket_create(ifname);
    if (sock < 0) return 1;

    printf("Захват на %s. Фильтр: %s. Нажмите Ctrl+C для выхода...\n\n", ifname, filter_str);

    unsigned char buffer[65536];
    time_t start_time = time(NULL);
    int packet_count = 0;

    /* 2. Бесконечный цикл чтения */
    while (1) {
        ssize_t data_size = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
        if (data_size < 0) {
            perror("recvfrom");
            break;
        }

        packet_info_t pkt;
        /* 3. Парсим пакет */
        if (parse_packet(buffer, (size_t)data_size, &pkt) == 0) {

            /* 4. Применяем фильтр */
            if (apply_filter(&pkt, filter_type)) {
                packet_count++;
                /* 5. Выводим на экран */
                print_packet_info(&pkt, start_time);
            }
        }
    }

    close(sock);
    printf("\nЗахвачено пакетов: %d\n", packet_count);
    return 0;
}