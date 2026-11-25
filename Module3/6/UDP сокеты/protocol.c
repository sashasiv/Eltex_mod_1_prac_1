/* ============================================================
 *  protocol.c — реализация length-prefixed протокола чата.
 *
 *  Ключевая идея: каждое поле предваряется десятичной длиной
 *  и символом '|'. Парсер читает длину, затем ровно N байт,
 *  поэтому содержимое поля не может «сломать» разбор.
 * ============================================================ */

#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

 /* ---------- Вспомогательные ---------- */

 /* Сформировать "<len>|<data>" в out.
  * Возврат: число записанных байт или -1, если не поместилось. */
static int put_field(char* out, size_t cap, const char* data, size_t len)
{
    int n = snprintf(out, cap, "%zu|", len);
    if (n < 0 || (size_t)n >= cap) return -1;
    if (n + len >= cap) return -1;
    memcpy(out + n, data, len);
    return n + (int)len;
}

/* Прочитать "<len>|<data>" из buf длиной buflen.
 * На успехе: *pos сдвигается за данные, *out_len = длина поля.
 * Возврат: указатель на начало данных внутри buf или NULL при ошибке. */
static const char* get_field(const char* buf, size_t buflen, size_t* pos,
    size_t* out_len)
{
    if (*pos >= buflen) return NULL;

    /* 1. Прочитать десятичное число до '|' */
    size_t start = *pos;
    size_t i = start;
    while (i < buflen && buf[i] >= '0' && buf[i] <= '9') i++;
    if (i == start) return NULL;          /* нет цифр */
    if (i >= buflen || buf[i] != '|') return NULL;  /* нет '|' */

    /* 2. Преобразовать число (защита от переполнения) */
    unsigned long long v = 0;
    for (size_t k = start; k < i; k++) {
        v = v * 10 + (unsigned)(buf[k] - '0');
        if (v > (unsigned long long)SIZE_MAX) return NULL;
    }
    size_t flen = (size_t)v;

    /* 3. Проверить, что данные помещаются в буфер */
    size_t data_start = i + 1;
    if (data_start + flen > buflen) return NULL;

    *out_len = flen;
    *pos = data_start + flen;
    return buf + data_start;
}

/* ---------- Формирование ---------- */

int proto_make_join(char* buf, size_t sz, unsigned id, const char* name)
{
    char idbuf[16];
    int idlen = snprintf(idbuf, sizeof(idbuf), "%u", id);
    if (idlen < 0) return -1;

    size_t pos = 0;

    /* префикс "JOIN|" */
    int n = snprintf(buf, sz, "%s|", PFX_JOIN);
    if (n < 0 || (size_t)n >= sz) return -1;
    pos = (size_t)n;

    /* id */
    n = put_field(buf + pos, sz - pos, idbuf, (size_t)idlen);
    if (n < 0) return -1;
    pos += (size_t)n;

    /* name */
    n = put_field(buf + pos, sz - pos, name, strlen(name));
    if (n < 0) return -1;
    pos += (size_t)n;

    return (int)pos;
}

int proto_make_leave(char* buf, size_t sz, unsigned id, const char* name)
{
    char idbuf[16];
    int idlen = snprintf(idbuf, sizeof(idbuf), "%u", id);
    if (idlen < 0) return -1;

    size_t pos = 0;
    int n = snprintf(buf, sz, "%s|", PFX_LEAVE);
    if (n < 0 || (size_t)n >= sz) return -1;
    pos = (size_t)n;

    n = put_field(buf + pos, sz - pos, idbuf, (size_t)idlen);
    if (n < 0) return -1;
    pos += (size_t)n;

    n = put_field(buf + pos, sz - pos, name, strlen(name));
    if (n < 0) return -1;
    pos += (size_t)n;

    return (int)pos;
}

int proto_make_msg(char* buf, size_t sz, unsigned id,
    const char* name, const char* text)
{
    char idbuf[16];
    int idlen = snprintf(idbuf, sizeof(idbuf), "%u", id);
    if (idlen < 0) return -1;

    size_t pos = 0;
    int n = snprintf(buf, sz, "%s|", PFX_MSG);
    if (n < 0 || (size_t)n >= sz) return -1;
    pos = (size_t)n;

    n = put_field(buf + pos, sz - pos, idbuf, (size_t)idlen);
    if (n < 0) return -1;
    pos += (size_t)n;

    n = put_field(buf + pos, sz - pos, name, strlen(name));
    if (n < 0) return -1;
    pos += (size_t)n;

    n = put_field(buf + pos, sz - pos, text, strlen(text));
    if (n < 0) return -1;
    pos += (size_t)n;

    return (int)pos;
}

/* ---------- Разбор ---------- */

int proto_parse(const char* buf, size_t len, message_t* out)
{
    memset(out, 0, sizeof(*out));
    out->type = MSG_INVALID;

    if (!buf || len == 0) { out->type = MSG_INVALID; return -1; }

    /* 1. Определить тип по префиксу до первого '|' */
    const char* bar = memchr(buf, '|', len);
    if (!bar) { out->type = MSG_INVALID; return -1; };
    size_t plen = (size_t)(bar - buf);

    if (plen == 4 && !memcmp(buf, PFX_JOIN, 4)) out->type = MSG_JOIN;
    else if (plen == 5 && !memcmp(buf, PFX_LEAVE, 5)) out->type = MSG_LEAVE;
    else if (plen == 3 && !memcmp(buf, PFX_MSG, 3)) out->type = MSG_CHAT;
    else { out->type = MSG_INVALID; return -1; }

    /* 2. Поля идут подряд: <len>|<data>... */
    size_t pos = plen + 1;   /* сразу за '|' */
    size_t flen = 0;
    const char* f = NULL;

    /* 2a. id */
    f = get_field(buf, len, &pos, &flen);
    if (!f || flen == 0 || flen > 10) { out->type = MSG_INVALID; return -1; }

    char idbuf[16];
    if (flen >= sizeof(idbuf)) { out->type = MSG_INVALID; return -1; }
    memcpy(idbuf, f, flen);
    idbuf[flen] = '\0';

    char* endptr = NULL;
    unsigned long v = strtoul(idbuf, &endptr, 10);
    if (!endptr || *endptr != '\0') { out->type = MSG_INVALID; return -1; }
    out->id = (unsigned)v;

    /* 2b. name */
    f = get_field(buf, len, &pos, &flen);
    if (!f) { out->type = MSG_INVALID; return -1; }
    if (flen >= NAME_LEN) flen = NAME_LEN - 1;   /* обрезаем, не падаем */
    memcpy(out->name, f, flen);
    out->name[flen] = '\0';

    if (out->name[0] == '\0') { out->type = MSG_INVALID; return -1; }          /* пустое имя — мусор */

    /* 2c. text (только для MSG) */
    if (out->type == MSG_CHAT) {
        f = get_field(buf, len, &pos, &flen);
        if (!f) { out->type = MSG_INVALID; return -1; }
        if (flen >= BUF_SIZE) flen = BUF_SIZE - 1;
        memcpy(out->text, f, flen);
        out->text[flen] = '\0';
    }

    return 0;
}