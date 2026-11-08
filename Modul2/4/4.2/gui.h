#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include "conveyor.h"

void gui_banner(void);
void gui_section(const char* title);
void gui_prompt(void);

/* снимок очереди: FIFO-порядок, политика, ready/blocked, статистика */
void gui_dump_queue(const Conveyor* q, PopPolicy policy, uint8_t min_p);

typedef struct {
    enum {
        CMD_NONE = 0,
        CMD_ADD,
        CMD_NEXT,
        CMD_POLICY_FIRST,
        CMD_POLICY_MIN,
        CMD_SIZE,
        CMD_HELP,
        CMD_QUIT,
        CMD_INVALID
    } kind;
    uint8_t prio;       /* CMD_ADD */
    uint8_t min_p;      /* CMD_POLICY_MIN */
    int     count;      /* CMD_NEXT: сколько задач выполнить */
    char    err[64];    /* CMD_INVALID */
} GuiCommand;

int gui_parse_command(const char* line, GuiCommand* cmd);

#endif