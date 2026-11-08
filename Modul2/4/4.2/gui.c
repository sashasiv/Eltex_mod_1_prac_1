#include "gui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void gui_banner(void) {
    printf("=== Conveyor: очередь с приоритетом ===\n");
    printf("CAP=%d, приоритет 0=высший, 255=низший\n", CV_CAP);
    printf("Команды: add <prio> | next [n] | policy first|min <p> | "
        "size | help | quit\n\n");
}

void gui_section(const char* title) {
    printf("--- %s ---\n", title);
}

void gui_prompt(void) {
    printf("> ");
    fflush(stdout);
}

static void print_policy(PopPolicy policy, uint8_t min_p) {
    if (policy == POP_FIRST) printf("policy : FIRST\n");
    else                     printf("policy : min(%u)\n", (unsigned)min_p);
}

static void print_stats(const Conveyor* q) {
    printf("queue  : %d/%d   push=%lu pop=%lu evict=%lu\n",
        cv_size(q), CV_CAP, q->pushes, q->pops, q->evictions);
}

static void print_buckets(const Conveyor* q, PopPolicy policy, uint8_t min_p) {
    int n_ready = 0, n_blocked = 0;

    printf("  [ready]\n");
    for (const ConveyorNode* n = cv_first_arr(q); n; n = cv_next_arr(n)) {
        int ok = (policy == POP_FIRST) || (n->prio >= min_p);
        if (ok) { printf("    id=%-3d prio=%u\n", n->id, n->prio); ++n_ready; }
    }
    if (!n_ready) printf("    — нет\n");

    printf("  [blocked]\n");
    for (const ConveyorNode* n = cv_first_arr(q); n; n = cv_next_arr(n)) {
        int ok = (policy == POP_FIRST) || (n->prio >= min_p);
        if (!ok) {
            printf("    id=%-3d prio=%u   (prio %u < min %u)\n",
                n->id, n->prio, n->prio, (unsigned)min_p);
            ++n_blocked;
        }
    }
    if (!n_blocked) printf("    — нет\n");
}

void gui_dump_queue(const Conveyor* q, PopPolicy policy, uint8_t min_p) {
    gui_section("очередь (порядок выполнения)");

    /* ready — в порядке выполнения: по возрастанию prio, FIFO среди равных.
     * Это ровно порядок приоритетного списка, но только элементы prio >= p
     * (при FIRST — все). Проходим приоритетный список один раз. */
    int k = 0;
    for (const ConveyorNode* n = cv_first_prio(q); n; n = cv_next_prio(n)) {
        int ok = (policy == POP_FIRST) || (n->prio >= min_p);
        if (ok) {
            printf("  %2d. id=%-3d prio=%u\n", ++k, n->id, n->prio);
        }
    }
    if (!k) printf("  (нет выполнимых)\n");

    print_policy(policy, min_p);
    print_stats(q);

    /* blocked — отдельно, без номеров */
    int n_blocked = 0;
    for (const ConveyorNode* n = cv_first_prio(q); n; n = cv_next_prio(n)) {
        int ok = (policy == POP_FIRST) || (n->prio >= min_p);
        if (!ok) {
            if (n_blocked == 0) printf("  [blocked]\n");
            printf("    id=%-3d prio=%u   (prio %u < min %u)\n",
                n->id, n->prio, n->prio, (unsigned)min_p);
            ++n_blocked;
        }
    }
    if (!n_blocked) printf("  [blocked] — нет\n");
}

/* ---------- разбор ---------- */

static int parse_int(const char* s, int* out) {
    char* end = NULL;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0') return 0;
    *out = (int)v;
    return 1;
}

int gui_parse_command(const char* line, GuiCommand* cmd) {
    memset(cmd, 0, sizeof * cmd);
    cmd->count = 1;

    while (*line && isspace((unsigned char)*line)) ++line;
    if (*line == '\0') { cmd->kind = CMD_NONE; return 1; }

    char word[32] = { 0 };
    int n = 0;
    while (*line && !isspace((unsigned char)*line) && n < (int)sizeof(word) - 1)
        word[n++] = *line++;
    word[n] = '\0';
    while (*line && isspace((unsigned char)*line)) ++line;

    if (!strcmp(word, "quit")) { cmd->kind = CMD_QUIT; return 1; }
    if (!strcmp(word, "size")) { cmd->kind = CMD_SIZE; return 1; }
    if (!strcmp(word, "help")) { cmd->kind = CMD_HELP; return 1; }

    if (!strcmp(word, "add")) {
        int prio;
        if (!*line || !parse_int(line, &prio) || prio < 0 || prio > 255) {
            cmd->kind = CMD_INVALID;
            snprintf(cmd->err, sizeof cmd->err, "add <prio 0..255>");
            return 1;
        }
        cmd->kind = CMD_ADD;
        cmd->prio = (uint8_t)prio;
        return 1;
    }

    if (!strcmp(word, "next")) {
        cmd->kind = CMD_NEXT;
        cmd->count = 1;
        if (*line) {
            int c;
            if (!parse_int(line, &c) || c <= 0) {
                cmd->kind = CMD_INVALID;
                snprintf(cmd->err, sizeof cmd->err, "next [n], n>0");
                return 1;
            }
            cmd->count = c;
        }
        return 1;
    }

    if (!strcmp(word, "policy")) {
        if (!strcmp(line, "first")) { cmd->kind = CMD_POLICY_FIRST; return 1; }
        if (!strncmp(line, "min", 3)) {
            const char* p = line + 3;
            while (*p && isspace((unsigned char)*p)) ++p;
            int mp;
            if (!parse_int(p, &mp) || mp < 0 || mp > 255) {
                cmd->kind = CMD_INVALID;
                snprintf(cmd->err, sizeof cmd->err, "policy min <0..255>");
                return 1;
            }
            cmd->kind = CMD_POLICY_MIN;
            cmd->min_p = (uint8_t)mp;
            return 1;
        }
        cmd->kind = CMD_INVALID;
        snprintf(cmd->err, sizeof cmd->err, "policy first | policy min <p>");
        return 1;
    }

    cmd->kind = CMD_INVALID;
    snprintf(cmd->err, sizeof cmd->err, "неизвестная команда");
    return 1;
}