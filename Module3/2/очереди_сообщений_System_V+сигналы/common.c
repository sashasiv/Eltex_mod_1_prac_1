#include "common.h"
#include <ctype.h>

/* ============================================================
 * имена команд
 * ============================================================ */

static const struct { cmd_t c; const char* s; } CMD_TAB[] = {
    { CMD_SEND,        "send" },
    { CMD_SUBSCRIBE,   "subscribe" },
    { CMD_UNSUBSCRIBE, "unsubscribe" },
    { CMD_LISTSUBS,    "listsubs" },
    { CMD_SUBS,        "subs" },
    { CMD_OK,          "ok" },
    { CMD_ERR,         "err" },
};

const char* cmd_name(cmd_t c)
{
    for (size_t i = 0; i < sizeof(CMD_TAB) / sizeof(CMD_TAB[0]); i++)
        if (CMD_TAB[i].c == c) return CMD_TAB[i].s;
    return "?";
}

cmd_t cmd_by_name(const char* s, size_t n)
{
    for (size_t i = 0; i < sizeof(CMD_TAB) / sizeof(CMD_TAB[0]); i++)
        if (strlen(CMD_TAB[i].s) == n && memcmp(CMD_TAB[i].s, s, n) == 0)
            return CMD_TAB[i].c;
    return CMD_UNKNOWN;
}

/* ============================================================
 * вспомогательное
 * ============================================================ */

static int parse_pid(const char* s, size_t n, pid_t* out)
{
    if (n == 0 || n >= 12) return -3;
    char buf[12];
    memcpy(buf, s, n);
    buf[n] = '\0';
    errno = 0;
    char* end = NULL;
    long v = strtol(buf, &end, 10);
    if (errno != 0 || end != buf + n || v <= 0 || v > 0x7fffffffL)
        return -3;
    *out = (pid_t)v;
    return 0;
}

/* топик: непустой, без , \n пробелов, не "*" */
static int valid_topic(const char* s, size_t n)
{
    if (n == 0 || n > TOPIC_MAX) return 0;
    if (n == 1 && s[0] == ARG_STAR[0]) return 0;
    for (size_t i = 0; i < n; i++) {
        unsigned char ch = (unsigned char)s[i];
        if (ch == ',' || ch == '\n' || isspace(ch)) return 0;
    }
    return 1;
}

/* ============================================================
 * парсер
 * ============================================================ */

int msg_parse(const char* mtext, size_t len, parsed_t* out)
{
    if (!mtext || !out || len == 0) return -1;
    memset(out, 0, sizeof(*out));

    const char* nl = memchr(mtext, '\n', len);
    size_t hdr_len = nl ? (size_t)(nl - mtext) : len;
    if (hdr_len == 0) return -1;

    const char* hdr_end = mtext + hdr_len;
    const char* c1 = memchr(mtext, ',', hdr_len);
    if (!c1) return -5;
    const char* c2 = memchr(c1 + 1, ',', hdr_end - (c1 + 1));
    if (!c2) return -5;
    if (memchr(c2 + 1, ',', hdr_end - (c2 + 1))) return -5;

    out->cmd = cmd_by_name(mtext, (size_t)(c1 - mtext));
    if (out->cmd == CMD_UNKNOWN) return -2;

    if (parse_pid(c1 + 1, (size_t)(c2 - c1 - 1), &out->pid) != 0)
        return -3;

    size_t alen = (size_t)(hdr_end - (c2 + 1));
    const char* argp = c2 + 1;

    switch (out->cmd) {
    case CMD_SEND:
    case CMD_SUBSCRIBE:
    case CMD_UNSUBSCRIBE:
        if (!valid_topic(argp, alen)) return -4;
        break;
    case CMD_LISTSUBS:
    case CMD_SUBS:
        if (!(alen == 1 && argp[0] == ARG_STAR[0])) return -4;
        break;
    case CMD_OK:
    case CMD_ERR:
        if (cmd_by_name(argp, alen) == CMD_UNKNOWN) return -4;
        break;
    default:
        return -2;
    }
    memcpy(out->arg, argp, alen);
    out->arg[alen] = '\0';

    /* payload — только send, err, subs; всегда с \n и \0 */
    if (out->cmd == CMD_SEND || out->cmd == CMD_ERR || out->cmd == CMD_SUBS) {
        if (nl) {
            out->payload = (char*)(nl + 1);
            out->payload_len = len - hdr_len - 1;
            if (out->payload_len > 0 &&
                out->payload[out->payload_len - 1] != '\0')
                return -1;
            if (out->payload_len > 0)
                out->payload_len--;
        }
        else {
            /* send: допустим без \n (пустой payload); err/subs — нет */
            if (out->cmd == CMD_ERR || out->cmd == CMD_SUBS) return -1;
            out->payload = NULL;
            out->payload_len = 0;
        }
    }
    else {
        /* для остальных хвост после \n игнорируется */
        out->payload = NULL;
        out->payload_len = 0;
    }
    return 0;
}

/* ============================================================
 * сборщики
 * ============================================================ */

int msg_build_send(struct msgbuf* m, pid_t from, const char* topic,
    const char* payload, size_t plen)
{
    if (!m || !topic) return -1;
    size_t tlen = strlen(topic);
    if (!valid_topic(topic, tlen)) return -1;
    if (plen > MSG_MAX - 64) return -1;

    int h = snprintf(m->mtext, MSG_MAX, "send,%d,%s\n", (int)from, topic);
    if (h < 0 || (size_t)h >= MSG_MAX) return -1;

    if (plen > 0) {
        if ((size_t)h + plen + 1 > MSG_MAX) return -1;
        memcpy(m->mtext + h, payload, plen);
    }
    m->mtext[h + plen] = '\0';
    m->mtype = MTYPE_TO_BROKER;
    return h + (int)plen + 1;
}

int msg_build_sub(struct msgbuf* m, pid_t from, const char* topic, int unsub)
{
    if (!m || !topic) return -1;
    size_t tlen = strlen(topic);
    if (!valid_topic(topic, tlen)) return -1;
    const char* cmd = unsub ? "unsubscribe" : "subscribe";
    int n = snprintf(m->mtext, MSG_MAX, "%s,%d,%s", cmd, (int)from, topic);
    if (n < 0 || n >= MSG_MAX) return -1;
    m->mtype = MTYPE_TO_BROKER;
    return n;
}

int msg_build_listsubs(struct msgbuf* m, pid_t from)
{
    if (!m) return -1;
    int n = snprintf(m->mtext, MSG_MAX, "listsubs,%d,%s", (int)from, ARG_STAR);
    if (n < 0 || n >= MSG_MAX) return -1;
    m->mtype = MTYPE_TO_BROKER;
    return n;
}

int msg_build_subs(struct msgbuf* m, pid_t to,
    char topics[][TOPIC_MAX + 1], int ntopics)
{
    if (!m || ntopics < 0) return -1;

    int h = snprintf(m->mtext, MSG_MAX, "subs,%d,%s\n", (int)to, ARG_STAR);
    if (h < 0 || (size_t)h >= MSG_MAX) return -1;

    size_t off = (size_t)h;
    for (int i = 0; i < ntopics; i++) {
        if (!topics) return -1;
        size_t tl = strlen(topics[i]);
        if (!valid_topic(topics[i], tl)) return -1;
        /* тема + \n (кроме последней) + \0 в конце */
        size_t need = tl + ((i + 1 < ntopics) ? 1 : 0) + 1;
        if (off + need > MSG_MAX) return -1;
        memcpy(m->mtext + off, topics[i], tl);
        off += tl;
        if (i + 1 < ntopics) m->mtext[off++] = '\n';
    }
    m->mtext[off++] = '\0';
    m->mtype = to;
    return (int)off;
}

int msg_build_ok(struct msgbuf* m, pid_t to, const char* command)
{
    if (!m || !command) return -1;
    if (cmd_by_name(command, strlen(command)) == CMD_UNKNOWN) return -1;
    int n = snprintf(m->mtext, MSG_MAX, "ok,%d,%s", (int)to, command);
    if (n < 0 || n >= MSG_MAX) return -1;
    m->mtype = to;
    return n;
}

int msg_build_err(struct msgbuf* m, pid_t to, const char* command,
    const char* errtext)
{
    if (!m || !command) return -1;
    if (cmd_by_name(command, strlen(command)) == CMD_UNKNOWN) return -1;
    size_t elen = errtext ? strlen(errtext) : 0;
    int h = snprintf(m->mtext, MSG_MAX, "err,%d,%s\n", (int)to, command);
    if (h < 0 || (size_t)h >= MSG_MAX) return -1;
    if ((size_t)h + elen + 1 > MSG_MAX) return -1;
    if (elen > 0) memcpy(m->mtext + h, errtext, elen);
    m->mtext[h + elen] = '\0';
    m->mtype = to;
    return h + (int)elen + 1;
}

/* ============================================================
 * разбор списка подписок (callback)
 * ============================================================ */

int parse_subs_topics(const char* payload, size_t plen,
    subs_topic_cb cb, void* ctx)
{
    if (!cb) return -1;
    if (plen == 0 || payload == NULL) return 0;

    int count = 0;
    size_t pos = 0;
    while (pos < plen) {
        /* пропускаем ведущие \n (пустые токены) */
        while (pos < plen && payload[pos] == '\n') pos++;
        if (pos >= plen) break;
        size_t start = pos;
        while (pos < plen && payload[pos] != '\n') pos++;
        size_t tlen = pos - start;
        if (tlen == 0) continue;
        if (tlen > TOPIC_MAX) return -1;
        cb(payload + start, tlen, ctx);
        count++;
    }
    return count;
}