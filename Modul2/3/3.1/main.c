#include "ui.h"
#include "core.h"

#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static void print_menu(void) {
    printf("\n=== Калькулятор маски прав доступа ===\n");
    printf("1. Ввести права (755 / rwxr-xr-- / -rwxr-xr--)\n");
    printf("2. Ввести имя файла (только на Линукс)\n");
    printf("3. Модифицировать права (chmod-команда или octal)\n");
    printf("0. Выход\n");
    printf("Выбор: ");
}

int main(void) {

    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif

    unsigned current_mode = 0;
    int have_mode = 0;
    char line[32];

    for (;;) {
        print_menu();

        if (fgets(line, sizeof line, stdin) == NULL) {
            printf("\nEOF, выход.\n");
            break;
        }

        /* обрезаем \n */
        size_t n = 0;
        while (line[n] && line[n] != '\n') ++n;
        line[n] = '\0';

        if (line[0] == '0' && line[1] == '\0') {
            printf("Выход.\n");
            break;
        }
        else if (line[0] == '1' && line[1] == '\0') {
            ui_prompt_mode(&current_mode, &have_mode);
        }
        else if (line[0] == '2' && line[1] == '\0') {
            ui_prompt_file(&current_mode, &have_mode);
        }
        else if (line[0] == '3' && line[1] == '\0') {
            ui_prompt_chmod(&current_mode, have_mode);
        }
        else if (line[0] == '\0') {
            /* пустой ввод — просто перерисовать меню */
        }
        else {
            printf("Неизвестный пункт меню.\n");
        }
    }

    return 0;
}