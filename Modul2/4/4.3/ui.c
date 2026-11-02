/* ui.c — ввод/вывод + main */
#include "phonebook.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static void _print_contact(const Contact* c)
{
    printf("%s %s %s\n", c->Fam, c->Im, c->Otch);
    if (c->workplace[0]) printf("  Работа:    %s\n", c->workplace);
    if (c->position[0])  printf("  Должность: %s\n", c->position);
    for (int i = 0; i < c->phones_count; ++i)      printf("  Тел:       %s\n", c->phones[i]);
    for (int i = 0; i < c->emails_count; ++i)      printf("  Email:     %s\n", c->emails[i]);
    for (int i = 0; i < c->socials_count; ++i)     printf("  Соцсеть:   %s\n", c->socials[i]);
    for (int i = 0; i < c->messengers_count; ++i)  printf("  Мессенджер:%s\n", c->messengers[i]);
}

static void _print_contact_cb(const Contact* c, void* ctx)
{
    (void)ctx;
    _print_contact(c);
    printf("\n");
}

static void _read(const char* prompt, char* buf, int size)
{
    printf("%s", prompt);
    if (!fgets(buf, size, stdin)) { buf[0] = '\0'; return; }
    size_t n = strlen(buf);
    if (n && buf[n - 1] == '\n') buf[n - 1] = '\0';
    else { int ch; while ((ch = getchar()) != '\n' && ch != EOF) {} }
}

static int _read_list(const char* label, char arr[][STR_LEN])
{
    char buf[16];
    printf("Сколько %s (0..%d): ", label, ARR_LEN);
    _read("", buf, sizeof(buf));
    int n = atoi(buf);
    if (n < 0) n = 0;
    if (n > ARR_LEN) n = ARR_LEN;
    for (int i = 0; i < n; ++i) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "%s #%d: ", label, i + 1);
        _read(prompt, arr[i], STR_LEN);
    }
    return n;
}

/* выбор контакта из снимка по префиксу фамилии; NULL — отмена/нет */
static TreeNode* ui_pick_contact_by_fam(void)
{
    char prefix[STR_LEN];
    _read("Фамилия (начало, Enter — отмена): ", prefix, sizeof(prefix));
    if (!prefix[0]) return NULL;

    Snapshot* s = snapshot_find_by_fam_prefix(prefix);
    if (!s) { printf("Не найдено.\n"); return NULL; }

    size_t n = snapshot_count(s);
    printf("Найдено %zu:\n", n);
    for (size_t i = 0; i < n; ++i) {
        TreeNode* node = snapshot_get(s, (int)(i + 1));
        const Contact* c = &node->data;
        printf("  %zu) %s %s %s | %s | %s\n",
            i + 1, c->Fam, c->Im,
            c->Otch[0] ? c->Otch : "—",
            c->workplace[0] ? c->workplace : "—",
            c->position[0] ? c->position : "—");
    }

    TreeNode* chosen = NULL;
    for (;;) {
        char buf[16];
        _read("Номер (0 — отмена): ", buf, sizeof(buf));
        int k = atoi(buf);
        if (k == 0) break;
        chosen = snapshot_get(s, k);
        if (chosen) break;
        printf("Неверный номер, повторите.\n");
    }

    snapshot_destroy(s);
    return chosen;
}

static void _edit_list(TreeNode* target, const char* label, const char* base)
{
    Contact* c = &target->data;

    char (*arr)[STR_LEN] = NULL;
    int count = 0;
    if (strcmp(base, "phone") == 0) { arr = c->phones;     count = c->phones_count; }
    else if (strcmp(base, "email") == 0) { arr = c->emails;     count = c->emails_count; }
    else if (strcmp(base, "social") == 0) { arr = c->socials;    count = c->socials_count; }
    else if (strcmp(base, "messenger") == 0) { arr = c->messengers; count = c->messengers_count; }

    printf("\n%s (%d):\n", label, count);
    if (count == 0) printf("  (пусто)\n");
    else for (int i = 0; i < count; ++i) printf("  %d) %s\n", i + 1, arr[i]);

    char op[16];
    _read("Операция: доб / уд / Enter — пропустить: ", op, sizeof(op));
    if (op[0] == '\0') return;

    char key[32];
    char value[STR_LEN];
    TreeNode* out = NULL;
    int rc;

    if (strcmp(op, "доб") == 0) {
        _read("Добавить: ", value, sizeof(value));
        snprintf(key, sizeof(key), "%s_add", base);
        rc = edit_contact(target, &out, key, value, NULL);
        if (rc != 0) printf("Добавить не удалось (код %d).\n", rc);
    }
    else if (strcmp(op, "уд") == 0) {
        if (count == 0) { printf("Список пуст — удалять нечего.\n"); return; }
        _read("Номер для удаления (1..N, 0 — отмена): ", value, sizeof(value));
        if (value[0] == '0') return;
        snprintf(key, sizeof(key), "%s_remove", base);
        rc = edit_contact(target, &out, key, value, NULL);
        if (rc != 0) printf("Удалить не удалось (код %d).\n", rc);
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

    phonebook_init();

    char Fam[STR_LEN], Im[STR_LEN], Otch[STR_LEN];
    char workplace[STR_LEN], position[STR_LEN];

    int Comand_name = 0;
    while (1)
    {
        if (Comand_name) {
            char buf[16];
            _read("Выбор: ", buf, sizeof(buf));
            Comand_name = atoi(buf);
        }
        switch (Comand_name) {
        default:
            printf("========Телефонный справочник========\n");
            printf("1 - Показать (LTR, по умолчанию)\n");
            printf("2 - Показать (TLR)\n");
            printf("3 - Показать (LRT)\n");
            printf("4 - Показать деревом\n");
            printf("5 - Добавить контакт\n");
            printf("6 - Изменить контакт\n");
            printf("7 - Удалить контакт\n");
            printf("-1 - Выход\n");
            break;
        case -1:
            return 0;
        case 1:
            if (contacts_count() == 0) { printf("Список пуст.\n"); break; }
            contacts_foreach(ORDER_LTR, _print_contact_cb, NULL);
            break;
        case 2:
            if (contacts_count() == 0) { printf("Список пуст.\n"); break; }
            contacts_foreach(ORDER_TLR, _print_contact_cb, NULL);
            break;
        case 3:
            if (contacts_count() == 0) { printf("Список пуст.\n"); break; }
            contacts_foreach(ORDER_LRT, _print_contact_cb, NULL);
            break;
        case 4:
            contacts_print_tree();
            break;
        case 5: {
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

            int rc = add_contact(Fam, Im,
                "Otch", Otch,
                "workplace", workplace,
                "position", position,
                NULL);
            if (rc == -1) { printf("Такой контакт уже есть.\n"); break; }
            if (rc == -2) { printf("Фамилия и имя обязательны.\n"); break; }
            if (rc != 0) { printf("Неизвестная ошибка (%d).\n", rc); break; }

            /* найти добавленный узел по ключу */
            Contact key; memset(&key, 0, sizeof(key));
            strncpy(key.Fam, Fam, STR_LEN - 1);
            strncpy(key.Im, Im, STR_LEN - 1);
            strncpy(key.Otch, Otch, STR_LEN - 1);
            strncpy(key.workplace, workplace, STR_LEN - 1);
            strncpy(key.position, position, STR_LEN - 1);

            TreeNode* node = find_node_by_key(&key);
            if (node) {
                TreeNode* out = NULL;
                for (int i = 0; i < phones_n; ++i)
                    edit_contact(node, &out, "phone_add", phones[i], NULL);
                for (int i = 0; i < emails_n; ++i)
                    edit_contact(node, &out, "email_add", emails[i], NULL);
                for (int i = 0; i < socials_n; ++i)
                    edit_contact(node, &out, "social_add", socials[i], NULL);
                for (int i = 0; i < messengers_n; ++i)
                    edit_contact(node, &out, "messenger_add", messengers[i], NULL);
            }
            printf("Добавлен контакт.\n");
            break;
        }
        case 6: {
            TreeNode* target = ui_pick_contact_by_fam();
            if (!target) break;
            _print_contact(&target->data);

            /* собрать изменённые пары (Enter — не менять) */
            const char* keys[5];
            const char* vals[5];
            int n = 0;

            _read("Фамилия   (Enter — не менять): ", Fam, sizeof(Fam));
            if (Fam[0]) { keys[n] = "Fam";       vals[n++] = Fam; }
            _read("Имя       (Enter — не менять): ", Im, sizeof(Im));
            if (Im[0]) { keys[n] = "Im";        vals[n++] = Im; }
            _read("Отчество  (Enter — не менять): ", Otch, sizeof(Otch));
            if (Otch[0]) { keys[n] = "Otch";      vals[n++] = Otch; }
            _read("Работа    (Enter — не менять): ", workplace, sizeof(workplace));
            if (workplace[0]) { keys[n] = "workplace"; vals[n++] = workplace; }
            _read("Должность (Enter — не менять): ", position, sizeof(position));
            if (position[0]) { keys[n] = "position";  vals[n++] = position; }

            /* один вызов со всеми парами — дубликат по итоговому ключу.
               edit_contact varargs — нельзя массив; поэтому вызываем по одной?
               Нет: используем varargs с фиксированным набором — но у нас n пар.
               Решение: если n>0 — вызвать edit_contact с n парами через
               отдельную функцию edit_contact_n (см. ниже) — либо собрать
               varargs вручную (невозможно). Поэтому добавим edit_contact_n
               в phonebook.h/.c — принимает keys/vals/n. */
            extern int edit_contact_n(TreeNode * target, TreeNode * *out,
                const char** keys, const char** vals, int n);
            TreeNode* out = NULL;
            int rc = 0;
            if (n > 0) rc = edit_contact_n(target, &out, keys, vals, n);

            if (rc == -1) { printf("Дубликат. Изменения отменены.\n"); break; }
            if (rc == -2) { printf("Фамилия и имя обязательны.\n"); break; }
            if (rc != 0) { printf("Неизвестная ошибка (%d).\n", rc); break; }

            if (out) target = out;

            _edit_list(target, "Телефоны", "phone");
            _edit_list(target, "Email", "email");
            _edit_list(target, "Соцсети", "social");
            _edit_list(target, "Мессенджеры", "messenger");

            _print_contact(&target->data);
            break;
        }
        case 7: {
            TreeNode* target = ui_pick_contact_by_fam();
            if (!target) break;
            _print_contact(&target->data);
            char buf[16];
            _read("Удалить контакт? (y/n): ", buf, sizeof(buf));
            if (buf[0] == 'y' || buf[0] == 'Y') {
                if (remove_contact(target)) printf("Контакт удалён.\n");
                else                        printf("Контакт не найден.\n");
            }
            break;
        }
        }
        Comand_name = -1;
    }
}