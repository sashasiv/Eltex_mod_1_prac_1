/* ui.c — ввод/вывод */
#include "phonebook.h"
#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static void _print_contact(const Contact* c)
{
    printf("[id=%d] %s %s %s\n", c->id, c->Fam, c->Im, c->Otch);
    if (c->workplace[0]) printf("  Работа:    %s\n", c->workplace);
    if (c->position[0])  printf("  Должность: %s\n", c->position);
    for (int i = 0; i < c->phones_count; ++i) printf("  Тел:       %s\n", c->phones[i]);
    for (int i = 0; i < c->emails_count; ++i) printf("  Email:     %s\n", c->emails[i]);
    for (int i = 0; i < c->socials_count; ++i) printf("  Соцсеть:   %s\n", c->socials[i]);
    for (int i = 0; i < c->messengers_count; ++i) printf("  Мессенджер:%s\n", c->messengers[i]);
}

static void _read(const char* prompt, char* buf, int size)
{
    printf("%s", prompt);
    if (!fgets(buf, size, stdin)) { buf[0] = '\0'; return; }
    size_t n = strlen(buf);
    if (n && buf[n - 1] == '\n') {
        buf[n - 1] = '\0';
    }
    else {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}
    }
}

static int _read_list(const char* label, char list[][STR_LEN])
{
    char buf[16];
    int n;

    printf("Сколько %s (0..%d): ", label, ARR_LEN);
    _read("", buf, sizeof(buf));
    n = atoi(buf);

    if (n < 0)       n = 0;
    if (n > ARR_LEN) n = ARR_LEN;

    for (int i = 0; i < n; ++i) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "%s #%d: ", label, i + 1);
        _read(prompt, list[i], STR_LEN);
    }
    return n;
}

static void _edit_list(int id, const char* label, const char* base)
{
    Contact* c = find_by_id(id);
    if (!c) return;

    /* выбрать список по base */
    char (*list)[STR_LEN] = NULL;
    int count = 0;
    if (strcmp(base, "phone") == 0) { list = c->phones;     count = c->phones_count; }
    else if (strcmp(base, "email") == 0) { list = c->emails;     count = c->emails_count; }
    else if (strcmp(base, "social") == 0) { list = c->socials;    count = c->socials_count; }
    else if (strcmp(base, "messenger") == 0) { list = c->messengers; count = c->messengers_count; }

    printf("\n%s (%d):\n", label, count);
    if (count == 0)
        printf("  (пусто)\n");
    else
        for (int i = 0; i < count; ++i)
            printf("  %d) %s\n", i + 1, list[i]);

    char op[16];
    _read("Операция: доб / уд / Enter — пропустить: ", op, sizeof(op));
    if (op[0] == '\0') return;

    char key[32];
    char value[STR_LEN];

    if (strcmp(op, "доб") == 0) {
        _read("Добавить: ", value, sizeof(value));
        snprintf(key, sizeof(key), "%s_add", base);
        if (!edit_contact(id, key, value, NULL)) printf("Добавить не удалось!\n");
    }
    else if (strcmp(op, "уд") == 0) {
        if (count == 0) { printf("Список пуст — удалять нечего.\n"); return; }
        _read("Номер для удаления (1..N, 0 — отмена): ", value, sizeof(value));
        if (value[0] == '0') return;
        snprintf(key, sizeof(key), "%s_remove", base);
        if (!edit_contact(id, key, value, NULL)) printf("Удалить не удалось!\n");
    }
    else {
        printf("Неизвестная операция '%s'.\n", op);
    }
}

int main(void)
{
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif

    char Fam[STR_LEN], Im[STR_LEN], Otch[STR_LEN];
    char workplace[STR_LEN], position[STR_LEN];

    int Comand_name = 0;
    while (true)
    {
        if (Comand_name) { // исключает первый вход
            char buf[16];
            _read("Выбор: ", buf, sizeof(buf));
            Comand_name = atoi(buf);
        }
        switch (Comand_name) {
        default:
            printf("========Телефонный справочник========\n");
            printf("1 - Показать контакты\n");
            printf("2 - Добавить контакт\n");
            printf("3 - Изменить контакт\n");
            printf("4 - Удалить контакт\n");
            printf("-1 - Выход\n");
            break;
        case -1:
            return 0;
        case 1: {
            int n = contacts_count();
            Contact* arr = contacts_array();
            if (!n) { printf("Список пуст.\n"); break; }
            for (int i = 0; i < n; ++i) {
                _print_contact(&arr[i]);
                printf("\n");
            }
            break;
        }
        case 2: {
            /* ---- добавление ---- */
            _read("Фамилия:    ", Fam, sizeof(Fam));
            _read("Имя:        ", Im, sizeof(Im));
            _read("Отчество:   ", Otch, sizeof(Otch));
            _read("Работа:     ", workplace, sizeof(workplace));
            _read("Должность:  ", position, sizeof(position));
                        
            char phones[ARR_LEN][STR_LEN];    int phones_n;
            char emails[ARR_LEN][STR_LEN];    int emails_n;
            char socials[ARR_LEN][STR_LEN];   int socials_n;
            char messengers[ARR_LEN][STR_LEN]; int messengers_n;

            phones_n = _read_list("телефонов", phones);
            emails_n = _read_list("email", emails);
            socials_n = _read_list("соцсетей", socials);
            messengers_n = _read_list("мессенджеров", messengers);

            int id = add_contact(Fam, Im,
                "Otch", Otch,
                "workplace", workplace,
                "position", position,
                NULL);
            if (id < 0) { printf("Ошибка: фамилия и имя обязательны.\n"); break; }

            for (int i = 0; i < phones_n; ++i)
                if (!edit_contact(id, "phone_add", phones[i], NULL))
                    printf("  Телефон #%d не добавлен (пустой или список полон).\n", i + 1);

            for (int i = 0; i < emails_n; ++i)
                if (!edit_contact(id, "email_add", emails[i], NULL))
                    printf("  Email #%d не добавлен (пустой или список полон).\n", i + 1);

            for (int i = 0; i < socials_n; ++i)
                if (!edit_contact(id, "social_add", socials[i], NULL))
                    printf("  Соцсеть #%d не добавлена (пустая или список полон).\n", i + 1);

            for (int i = 0; i < messengers_n; ++i)
                if (!edit_contact(id, "messenger_add", messengers[i], NULL))
                    printf("  Мессенджер #%d не добавлен (пустой или список полон).\n", i + 1);
            printf("Добавлен контакт id=%d\n", id);
            break;
        }
        case 3: {
            char buf[16];
            _read("Введите id контакта: ", buf, sizeof(buf));
            int id = atoi(buf);

            Contact* c = find_by_id(id);
            if (!c) {
                printf("Контакт id=%d не найден.\n", id);
                break;
            }
            _print_contact(c);

            /* ---- скаляры: Enter — не менять ---- */
            _read("Фамилия   (Enter — не менять): ", Fam, sizeof(Fam));
            if (Fam[0])      if (!edit_contact(id, "Fam", Fam, NULL))
            {
                printf("Ошибка ввода начните сначала!\n");
                break;
            }
         
            _read("Имя       (Enter — не менять): ", Im, sizeof(Im));
            if (Im[0])        if(!edit_contact(id, "Im", Im, NULL))
            {
                printf("Ошибка ввода начните сначала!\n");
                break;
            }
        
            _read("Отчество  (Enter — не менять): ", Otch, sizeof(Otch));
            if (Otch[0])      edit_contact(id, "Otch", Otch, NULL);

            _read("Работа    (Enter — не менять): ", workplace, sizeof(workplace));
            if (workplace[0]) edit_contact(id, "workplace", workplace, NULL);

            _read("Должность (Enter — не менять): ", position, sizeof(position));
            if (position[0])  edit_contact(id, "position", position, NULL);

            /* ---- списки: add / remove ---- */
            _edit_list(id, "Телефоны", "phone");
            _edit_list(id, "Email", "email");
            _edit_list(id, "Соцсети", "social");
            _edit_list(id, "Мессенджеры", "messenger");

            /* ---- показать результат ---- */
            c = find_by_id(id);           /* перечитать: могли удалить/добавить */
            if (c) _print_contact(c);

            break;
        }
        case 4: {
            /* ---- удаление ---- */
            char buf[16];
            _read("Введите id контакта: ", buf, sizeof(buf));   // 1) прочитать строку
            int id = atoi(buf);

            Contact* c = find_by_id(id);

            if (!c) {
                printf("Контакт id=%d не найден.\n", id);
                break;
            }
            _print_contact(c);

            _read("Удалить контакт? (y/n): ", buf, sizeof(buf));
            if (buf[0] == 'y' || buf[0] == 'Y'){
                if (remove_contact(id)) printf("Контакт id=%d удалён.\n", id);
                else                    printf("Контакт id=%d не найден.\n", id);
            }
            break;
        }

        }
        Comand_name = -1;
    }
}