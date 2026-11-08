#include "calc.h"
#include "loader.h"
#include <string.h>

/* ==================== Приём/выдача ==================== */

void print_menu(const Command* cmds, int n) {
    printf("\n=== КАЛЬКУЛЯТОР ===\n");
    for (int i = 0; i < n; i++) {
        if (cmds[i].kind == CMD_BINARY)
            printf("  a %c b  — %s\n", cmds[i].name, cmds[i].descr);
        else
            printf("  %c a    — %s\n", cmds[i].name, cmds[i].descr);
    }
    printf("  q      — выход\n");
    printf("  reload — обновить плагины\n");
}

int read_line(char* buf, int size) {
    if (!fgets(buf, size, stdin)) return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    return 1;
}

/* ==================== main ==================== */

int main(int argc, char** argv) {
    const char* dir = (argc > 1) ? argv[1] : "plugins";

    Command cmds[MAX_COMMANDS];
    LoadedPlugin plugins[MAX_COMMANDS];
    int n_cmds = 0, n_plugins = 0;
    char err[MAX_ERR] = { 0 };

    // Первичная загрузка
    if (!load_plugins(dir, cmds, &n_cmds, plugins, MAX_COMMANDS, &n_plugins, err, sizeof err)) {
        fprintf(stderr, "Ошибка загрузки: %s\n", err);
        return 1;
    }

    printf("Загружено функций: %d\n", n_cmds);
    print_menu(cmds, n_cmds);

    char line[MAX_LINE];
    Token tokens[MAX_TOKENS];

    for (;;) {
        printf("\nВведите выражение, 'reload' для обновления или q: ");
        if (!read_line(line, sizeof line)) break;

        if (strcmp(line, "q") == 0 || strcmp(line, "Q") == 0) break;

        // --- ЛОГИКА ПЕРЕЗАГРУЗКИ ---
        if (strcmp(line, "reload") == 0) {
            printf("Обновление плагинов из '%s'...\n", dir);

            Command new_cmds[MAX_COMMANDS];
            LoadedPlugin new_plugins[MAX_COMMANDS];
            int new_n_cmds = 0, new_n_plugins = 0;
            char reload_err[MAX_ERR] = { 0 };

            if (load_plugins(dir, new_cmds, &new_n_cmds, new_plugins, MAX_COMMANDS, &new_n_plugins, reload_err, sizeof reload_err)) {
                // Успех! Выгружаем старые
                unload_plugins(plugins, n_plugins);
                
                // Копируем новые данные
                memcpy(cmds, new_cmds, sizeof(Command) * new_n_cmds);
                memcpy(plugins, new_plugins, sizeof(LoadedPlugin) * new_n_plugins);
                n_cmds = new_n_cmds;
                n_plugins = new_n_plugins;

                printf("Успешно обновлено! Загружено функций: %d\n", n_cmds);
                print_menu(cmds, n_cmds);
            } else {
                // Если не удалось загрузить, закрываем частично загруженные
                unload_plugins(new_plugins, new_n_plugins);
                printf("Ошибка обновления: %s\n", reload_err);
                printf("Оставлены старые функции (%d шт.).\n", n_cmds);
            }
            continue;
        }
        // ---------------------------

        if (line[0] == '\0') continue;

        int tc = 0;
        double value = 0;
        err[0] = '\0';

        if (!tokenize(line, cmds, n_cmds, tokens, MAX_TOKENS, &tc, err, sizeof err)) {
            printf("Ошибка: %s\n", err);
            continue;
        }
        if (!parse_expression(tokens, tc, &value, err, sizeof err)) {
            printf("Ошибка: %s\n", err);
            continue;
        }
        printf("Результат: %g\n", value);
    }

    unload_plugins(plugins, n_plugins);
    printf("До свидания!\n");
    return 0;
}