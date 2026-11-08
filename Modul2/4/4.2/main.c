#include "conveyor.h"
#include "gui.h"

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static void print_help(void) {
    gui_section("команды");
    printf("  add <prio>         добавить задачу (prio 0..255)\n");
    printf("  next [n]           выполнить n задач (по умолч. 1) по текущей политике\n");
    printf("  policy first       выборка: самый важный\n");
    printf("  policy min <p>     выборка: самый важный среди prio >= p\n");
    printf("  size               показать очередь\n");
    printf("  help               эта справка\n");
    printf("  quit               выход\n");
}

int main(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    Conveyor q;
    cv_init(&q);

    PopPolicy policy = POP_FIRST;
    uint8_t   policy_p = 0;
    int       next_id = 1;

    char line[256];

    gui_banner();
    gui_section("режим: операции над очередью");

    for (;;) {
        gui_prompt();
        if (!fgets(line, sizeof line, stdin)) break;

        size_t L = strlen(line);
        while (L > 0 && (line[L - 1] == '\n' || line[L - 1] == '\r'))
            line[--L] = '\0';

        GuiCommand cmd;
        gui_parse_command(line, &cmd);

        switch (cmd.kind) {
        case CMD_NONE:
            break;

        case CMD_ADD: {
            int id = next_id++;
            int evicted = -1;
            cv_push(&q, id, cmd.prio, &evicted);
            printf("added  id=%d prio=%u\n", id, cmd.prio);
            if (evicted >= 0)
                printf("evict  id=%d (переполнение, самая старая)\n", evicted);
            break;
        }

        case CMD_NEXT: {
            int n = (cmd.count > 0) ? cmd.count : 1;
            for (int i = 0; i < n; ++i) {
                if (cv_is_empty(&q)) { printf("empty  (очередь пуста)\n"); break; }
                ConveyorNode* node = (policy == POP_FIRST)
                    ? cv_pop_first(&q)
                    : cv_pop_min_priority(&q, policy_p);
                if (!node) { printf("empty  (нет подходящих под политику)\n"); break; }
                printf("done   id=%d prio=%u\n", node->id, node->prio);
                cv_release(&q, node);
            }
            gui_dump_queue(&q, policy, policy_p);
            break;
        }

        case CMD_POLICY_FIRST:
            policy = POP_FIRST; policy_p = 0;
            printf("policy = FIRST\n");
            break;

        case CMD_POLICY_MIN:
            policy = POP_MIN_PRIO; policy_p = cmd.min_p;
            printf("policy = min(%u)\n", (unsigned)policy_p);
            break;

        case CMD_SIZE:
            gui_dump_queue(&q, policy, policy_p);
            break;

        case CMD_HELP:    print_help(); break;
        case CMD_INVALID: printf("ошибка: %s\n", cmd.err); break;
        case CMD_QUIT:    goto done;
        }
    }

done:
    printf("bye\n");
    return 0;
}