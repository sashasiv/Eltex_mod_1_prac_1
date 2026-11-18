#ifndef COMMON_H
#define COMMON_H

#include "config.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* ---------- команды ---------- */

typedef enum {
    CMD_UNKNOWN = 0,
    CMD_SEND,
    CMD_SUBSCRIBE,
    CMD_UNSUBSCRIBE,
    CMD_LISTSUBS,
    CMD_SUBS,
    CMD_OK,
    CMD_ERR
} cmd_t;

extern const char* cmd_name(cmd_t c);
extern cmd_t      cmd_by_name(const char* s, size_t n);

/* ---------- направление ---------- */

/* что брокер вправе получить (mtype = MTYPE_TO_BROKER) */
static inline int cmd_to_broker(cmd_t c) {
    return c == CMD_SEND || c == CMD_SUBSCRIBE ||
        c == CMD_UNSUBSCRIBE || c == CMD_LISTSUBS;
}
/* что подписчик вправе получить (mtype = его pid) */
static inline int cmd_to_subscriber(cmd_t c) {
    return c == CMD_SEND || c == CMD_SUBS ||
        c == CMD_OK || c == CMD_ERR;
}

/* ---------- разобранное сообщение ---------- */

typedef struct {
    cmd_t   cmd;
    pid_t   pid;
    char    arg[TOPIC_MAX + 1];   /* топик | имя команды | "*" */
    char* payload;              /* send, err, subs — после \n; иначе NULL */
    size_t  payload_len;
} parsed_t;

struct msgbuf {
    long mtype;
    char mtext[MSG_MAX];
};

/* ---------- парсер ---------- */

/* 0 — OK; отрицательный код ошибки */
int msg_parse(const char* mtext, size_t len, parsed_t* out);

/* ---------- сборщики ---------- */

int msg_build_send(struct msgbuf* m, pid_t from, const char* topic,
    const char* payload, size_t plen);
int msg_build_sub(struct msgbuf* m, pid_t from, const char* topic, int unsub);
int msg_build_listsubs(struct msgbuf* m, pid_t from);
int msg_build_subs(struct msgbuf* m, pid_t to,
    char topics[][TOPIC_MAX + 1], int ntopics);
int msg_build_ok(struct msgbuf* m, pid_t to, const char* command);
int msg_build_err(struct msgbuf* m, pid_t to, const char* command,
    const char* errtext);

/* ---------- разбор списка подписок (callback) ---------- */

typedef void (*subs_topic_cb)(const char* topic, size_t len, void* ctx);

/* вызывает cb для каждой темы; возвращает число тем или -1 */
int parse_subs_topics(const char* payload, size_t plen,
    subs_topic_cb cb, void* ctx);

#endif