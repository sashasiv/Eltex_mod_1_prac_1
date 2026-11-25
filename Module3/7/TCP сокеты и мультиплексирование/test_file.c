#include "protocol.h"
#include "chat.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    printf("=== Тест FILE ===\n");

    char buf[BUF_SIZE];

    // Формируем ровно то, что было в логе
    int len = proto_make_file(buf, sizeof(buf), 1795521131, "tu1", "one.txt", 1);

    printf("Строка (%d байт): %.*s\n", len, len, buf);

    message_t m;
    int res = proto_parse(buf, (size_t)len, &m);

    printf("Результат парсинга: %d\n", res);

    if (res == 0) {
        printf("Тип: %d (ожидали %d)\n", m.type, MSG_FILE);
        printf("ID: %u\n", m.id);
        printf("Имя: '%s'\n", m.name);
        printf("Файл: '%s'\n", m.text);
        printf("Размер: %zu\n", m.file_size);
    }
    else {
        printf("ОШИБКА ПАРСИНГА!\n");
    }

    return 0;
}