#include "ui.h"
#include "core.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

/* Вспомогательная функция: разбор количества пакетов N.
   Возвращает true при успехе, false при ошибке. */
static bool parse_count(const char* s, int* out) {
    if (s == NULL || *s == '\0') return false;

    errno = 0;
    char* end = NULL;
    long val = strtol(s, &end, 10);

    /* Ошибка переполнения, мусор в конце или пустая строка */
    if (errno != 0 || end == s || *end != '\0') return false;

    /* Ограничим N разумными пределами, чтобы программа не зависла */
    if (val <= 0 || val > 10000000) return false;

    *out = (int)val;
    return true;
}

bool ui_run(int argc, char** argv) {
    /* 1. Проверка количества аргументов */
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <IP шлюза> <маска подсети> <N пакетов>\n", argv[0]);
        fprintf(stderr, "Пример: %s 192.168.1.1 255.255.255.0 1000\n", argv[0]);
        return false;
    }

    uint32_t gw_ip = 0;
    uint32_t mask = 0;
    int n = 0;

    /* 2. Разбор IP шлюза */
    parse_ip_status_t st = parse_ipv4(argv[1], &gw_ip);
    if (st != PARSE_IP_OK) {
        fprintf(stderr, "Ошибка IP шлюза: %s\n", parse_ip_status_str(st));
        return false;
    }

    /* 3. Разбор маски */
    st = parse_ipv4(argv[2], &mask);
    if (st != PARSE_IP_OK) {
        fprintf(stderr, "Ошибка маски: %s\n", parse_ip_status_str(st));
        return false;
    }

    /* Проверка, что маска непрерывна (например, 255.255.255.0) */
    if (!is_valid_mask(mask)) {
        fprintf(stderr, "Ошибка маски: %s\n", parse_ip_status_str(PARSE_IP_ERR_MASK));
        return false;
    }

    /* 4. Разбор N */
    if (!parse_count(argv[3], &n)) {
        fprintf(stderr, "Ошибка количества пакетов: ожидается целое число от 1 до 10 000 000\n");
        return false;
    }

    /* 5. Подготовка к имитации */
    uint32_t network = subnet_address(gw_ip, mask);
    random_init();

    int own_count = 0;
    int foreign_count = 0;

    /* 6. Цикл имитации */
    for (int i = 0; i < n; ++i) {
        uint32_t dst_ip = random_ipv4();
        if (is_in_subnet(dst_ip, network, mask)) {
            own_count++;
        }
        else {
            foreign_count++;
        }
    }

    /* 7. Вывод результатов */
    char gw_str[16], mask_str[16], net_str[16];
    format_ipv4(gw_ip, gw_str);
    format_ipv4(mask, mask_str);
    format_ipv4(network, net_str);

    printf("\n=== Имитация маршрутизации IPv4 ===\n");
    printf("Шлюз:    %s\n", gw_str);
    printf("Маска:   %s\n", mask_str);
    printf("Подсеть: %s\n", net_str);
    printf("\nОбработано пакетов: %d\n", n);

    double pct_own = ((double)own_count / n) * 100.0;
    double pct_foreign = ((double)foreign_count / n) * 100.0;

    printf("-----------------------------------\n");
    printf("Своя подсеть: %d (%.2f%%)\n", own_count, pct_own);
    printf("Другие сети:  %d (%.2f%%)\n", foreign_count, pct_foreign);

    return 0;
}