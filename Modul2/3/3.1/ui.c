#define _CRT_SECURE_NO_WARNINGS
#include "ui.h"
#include "core.h"

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Обрезать завершающий '\n' (от fgets) и, на всякий случай, '\r' (Windows-перенос). */
static void chomp(char* s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

/* Прочитать строку с stdin в buf (размер cap). Возвращает 0 при успехе, -1 при EOF/ошибке. */
static int read_line(char* buf, size_t cap) {
    if (fgets(buf, (int)cap, stdin) == NULL) return -1;
    chomp(buf);
    return 0;
}

/*
 * Печать mode во всех трёх представлениях.
 * Формат:
 *   букв.:  rwxr-xr--
 *   цифр.:  0754
 *   биты:   000 111 101 100
 */
void ui_print_mode(unsigned mode) {
    char sym[10];
    char oct[5];
    char bin[16];

    mode_to_symbolic(mode, sym);
    mode_to_octal(mode, oct);
    mode_to_binary(mode, bin);

    printf("  букв.:  %s\n", sym);
    printf("  цифр.:  %s\n", oct);
    printf("  биты:   %s\n", bin);
}

/*
 * Пункт 1: ввод прав и вывод представлений.
 * Возвращает 0 при успехе, -1 при ошибке.
 */
int ui_prompt_mode(unsigned* current_mode, int* have_mode) {
    char buf[64];

    printf("Введите права доступа (например, 755, 0755, rwxr-xr--, -rwxr-xr--)\n");
    printf("> ");

    if (read_line(buf, sizeof buf) != 0) {
        printf("Ошибка чтения ввода.\n");
        return -1;
    }

    if (buf[0] == '\0') {
        printf("Пустой ввод.\n");
        return -1;
    }

    unsigned mode = 0;
    parse_status_t st = mode_parse(buf, &mode);
    if (st != PARSE_OK) {
        printf("Ошибка разбора: %s\n", parse_status_str(st));
        return -1;
    }

    /* успех: сохраняем состояние для п.3 */
    *current_mode = mode;
    *have_mode = 1;

    printf("Права доступа:\n");
    ui_print_mode(mode);
    return 0;
}

/* Тип файла из st_mode -> символ для буквенной записи (как ls -l). */
#ifdef __linux__
static char file_type_char(unsigned m) {
    switch (m & 0170000) {
    case 0100000: return '-';   /* S_IFREG */
    case 0040000: return 'd';   /* S_IFDIR */
    case 0120000: return 'l';   /* S_IFLNK */
    case 0020000: return 'c';   /* S_IFCHR */
    case 0060000: return 'b';   /* S_IFBLK */
    case 0010000: return 'p';   /* S_IFIFO */
    case 0140000: return 's';   /* S_IFSOCK */
    default:      return '?';
    }
}
#endif

int ui_prompt_file(unsigned* current_mode, int* have_mode) {
#ifdef __linux__
    char path[1024+32];

    printf("Введите имя файла (путь):\n");
    printf("> ");

    if (read_line(path, sizeof path) != 0) {
        printf("Ошибка чтения ввода.\n");
        return -1;
    }
    if (path[0] == '\0') {
        printf("Пустой ввод.\n");
        return -1;
    }

    struct stat st;
    if (lstat(path, &st) != 0) {
        printf("lstat(\"%s\") не удался: %s\n", path, strerror(errno));
        return -1;
    }

    unsigned mode = (unsigned)(st.st_mode & 07777);
    char ftype = file_type_char(st.st_mode);

    /* сохраняем состояние для п.3 */
    *current_mode = mode;
    *have_mode = 1;

    printf("Файл: %s\n", path);
    printf("  тип:    %c (%s)\n", ftype,
        S_ISDIR(st.st_mode) ? "каталог" :
        S_ISREG(st.st_mode) ? "обычный файл" :
        S_ISLNK(st.st_mode) ? "симлинк" :
        S_ISCHR(st.st_mode) ? "символьное устройство" :
        S_ISBLK(st.st_mode) ? "блочное устройство" :
        S_ISFIFO(st.st_mode) ? "FIFO" :
        S_ISSOCK(st.st_mode) ? "сокет" : "неизвестно");

    /* буквенное с типом (как ls -l) и без типа */
    char sym10[11];
    char sym9[10];
    mode_to_symbolic(mode, sym9);
    sym10[0] = ftype;
    memcpy(sym10 + 1, sym9, 9);
    sym10[10] = '\0';

    char oct[5];
    char bin[16];
    mode_to_octal(mode, oct);
    mode_to_binary(mode, bin);

    printf("Права доступа:\n");
    printf("  букв.:  %s   (%s)\n", sym10, sym9);
    printf("  цифр.:  %s\n", oct);
    printf("  биты:   %s\n", bin);

    /* сравнение с ls -l */
    printf("Сравнение с ls -l:\n");
    char cmd[1024];

    snprintf(cmd, sizeof cmd, "ls -ld -- \"%s\"", path);

    printf("  $ %s\n", cmd);
    fflush(stdout);

    int rc = system(cmd);
    if (rc == -1) {
        printf("  (не удалось запустить ls: %s)\n", strerror(errno));
    }

    return 0;
#else 
    (void)current_mode;
    (void)have_mode;
    printf("Пункт 2 доступен только на Linux.\n");
    return -1;
#endif
}

/* Пункт 3: запросить chmod-команду или octal, применить в памяти,
   вывести новое представление. Файл на диске НЕ меняется. */
int ui_prompt_chmod(unsigned* current_mode, int have_mode) {
    char buf[64];

    if (!have_mode) {
        printf("Сначала введите права (п.1) или файл (п.2).\n");
        return -1;
    }

    printf("Текущие права:\n");
    ui_print_mode(*current_mode);

    printf("Введите chmod-команду (u+x, go-w, a=r, u=rw,g=r,o=)\n"
        "или восьмеричную запись (755, 0755, 4777):\n");
    printf("> ");

    if (read_line(buf, sizeof buf) != 0) {
        printf("Ошибка чтения ввода.\n");
        return -1;
    }
    if (buf[0] == '\0') {
        printf("Пустой ввод.\n");
        return -1;
    }

    unsigned new_mode = 0;
    parse_status_t st = mode_apply_chmod(*current_mode, buf, &new_mode);
    if (st != PARSE_OK) {
        printf("Ошибка разбора команды: %s\n", parse_status_str(st));
        return -1;
    }

    *current_mode = new_mode;

    printf("Новые права (файл НЕ изменён):\n");
    ui_print_mode(new_mode);
    return 0;
}