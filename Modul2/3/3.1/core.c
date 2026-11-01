#include "core.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

const char* parse_status_str(parse_status_t st) {
    switch (st) {
    case PARSE_OK:          return "OK";
    case PARSE_ERR_FORMAT:  return "неверный формат/длина";
    case PARSE_ERR_DIGIT:   return "не восьмеричная цифра (0..7)";
    case PARSE_ERR_CHAR:    return "недопустимый символ в записи прав";
    default:                return "неизвестная ошибка";
    }
}

parse_status_t mode_from_octal(const char* s, unsigned* mode_out) {
    if (s == NULL || *s == '\0') return PARSE_ERR_FORMAT;

    const char* p = s;

    /* необязательный ведущий ноль (ровно один) */
    if (*p == '0') ++p;

    /* восьмеричные цифры */
    int digits = 0;
    unsigned m = 0;
    for (; *p; ++p) {
        if (*p < '0' || *p > '7') return PARSE_ERR_DIGIT;
        if (++digits > 4) return PARSE_ERR_FORMAT;   /* > 07777 не влезает */
        m = (m << 3) | (unsigned)(*p - '0');
    }

    *mode_out = m;
    return PARSE_OK;
}

parse_status_t mode_from_symbolic(const char* s, unsigned* mode_out) {
    if (s == NULL || mode_out == NULL) return PARSE_ERR_FORMAT;

    size_t len = strlen(s);
    const char* perms;

    if (len == 9) {
        perms = s;
    }
    else if (len == 10) {
        switch (s[0]) {
        case '-': case 'd': case 'l':
        case 'c': case 'b': case 'p': case 's':
            perms = s + 1;      /* отбрасываем тип файла */
            break;
        default:
            return PARSE_ERR_CHAR;   /* недопустимый тип файла */
        }
    }
    else {
        return PARSE_ERR_FORMAT;         /* не 9 и не 10 символов */
    }

    unsigned m = 0;
    for (int i = 0; i < 9; ++i) {
        unsigned bit = 1u << (8 - i);    /* 0400 .. 0001 */
        char expect = "rwx"[i % 3];      /* ожидаемый символ, если бит стоит */
        char c = perms[i];

        if (c == expect) {
            m |= bit;                    /* обычный бит r/w/x */
        }
        else if (c == '-') {
            /* бит не установлен */
        }
        else if (i == 2 && (c == 's' || c == 'S')) {   /* SUID */
            m |= 04000;
            if (c == 's') m |= 0100;     /* 's' = SUID + owner-x */
        }
        else if (i == 5 && (c == 's' || c == 'S')) {   /* SGID */
            m |= 02000;
            if (c == 's') m |= 0010;     /* 's' = SGID + group-x */
        }
        else if (i == 8 && (c == 't' || c == 'T')) {   /* sticky */
            m |= 01000;
            if (c == 't') m |= 0001;     /* 't' = sticky + other-x */
        }
        else {
            return PARSE_ERR_CHAR;       /* недопустимый символ/позиция */
        }
    }

    *mode_out = m;
    return PARSE_OK;
}

parse_status_t mode_parse(const char* s, unsigned* mode_out) {
    if (s == NULL || mode_out == NULL) return PARSE_ERR_FORMAT;
    if (*s == '\0') return PARSE_ERR_FORMAT;

    char c = s[0];

    if (c >= '0' && c <= '9')
        return mode_from_octal(s, mode_out);

    switch (c) {
    case '-': case 'd': case 'l':
    case 'c': case 'b': case 'p': case 's':
    case 'r':
        return mode_from_symbolic(s, mode_out);
    default:
        return PARSE_ERR_CHAR;   /* сразу ошибка, без захода в парсеры */
    }
}

void mode_to_symbolic(unsigned mode, char out[10]) {
    const char* rwx = "rwx";
    for (int i = 0; i < 9; ++i)
        out[i] = (mode & (1u << (8 - i))) ? rwx[i % 3] : '-';

    if (mode & 04000) out[2] = (mode & 0100) ? 's' : 'S';   /* SUID */
    if (mode & 02000) out[5] = (mode & 0010) ? 's' : 'S';   /* SGID */
    if (mode & 01000) out[8] = (mode & 0001) ? 't' : 'T';   /* sticky */

    out[9] = '\0';
}

void mode_to_octal(unsigned mode, char out[5]) {
    snprintf(out, 5, "%04o", mode & 07777);
}

void mode_to_binary(unsigned mode, char out[16]) {
    unsigned m = mode & 07777;
    snprintf(out, 16, "%c%c%c %c%c%c %c%c%c %c%c%c",
        (m & 04000) ? '1' : '0',
        (m & 02000) ? '1' : '0',
        (m & 01000) ? '1' : '0',
        (m & 0400) ? '1' : '0',
        (m & 0200) ? '1' : '0',
        (m & 0100) ? '1' : '0',
        (m & 0040) ? '1' : '0',
        (m & 0020) ? '1' : '0',
        (m & 0010) ? '1' : '0',
        (m & 0004) ? '1' : '0',
        (m & 0002) ? '1' : '0',
        (m & 0001) ? '1' : '0');
}

/* ---------- Пункт 3: chmod-подобная модификация ---------- */

/* Применить ОДНУ chmod-команду (без запятых) к *mode. */
static parse_status_t chmod_one(unsigned* mode, const char* cmd) {
    const char* p = cmd;

    /* 1. категории: u g o a (возможно несколько) */
    unsigned who = 0;   /* 1=u, 2=g, 4=o */
    while (*p == 'u' || *p == 'g' || *p == 'o' || *p == 'a') {
        switch (*p) {
        case 'u': who |= 1; break;
        case 'g': who |= 2; break;
        case 'o': who |= 4; break;
        case 'a': who |= 7; break;
        }
        ++p;
    }
    if (who == 0) who = 7;   /* без категории = a (как chmod +x) */

    /* 2. операция: + - = (ровно одна) */
    char op = *p;
    if (op != '+' && op != '-' && op != '=') return PARSE_ERR_CHAR;
    ++p;

    /* 3. права: r w x s t (возможно несколько; для '=' можно пусто) */
    unsigned rwx = 0;                    /* 4=r, 2=w, 1=x */
    int set_suid = 0, set_sgid = 0, set_sticky = 0;

    for (; *p; ++p) {
        switch (*p) {
        case 'r': rwx |= 4; break;
        case 'w': rwx |= 2; break;
        case 'x': rwx |= 1; break;
        case 's':
            if (who & 4) return PARSE_ERR_CHAR;   /* s с o — всегда ошибка */
            if (who & 1) set_suid = 1;
            if (who & 2) set_sgid = 1;
            break;
        case 't':
            set_sticky = 1;              /* sticky — глобальный бит */
            break;
        default:
            return PARSE_ERR_CHAR;
        }
    }

    /* 4. применить */
    unsigned m = *mode;

    /* 4a. для '=' сначала сбросить rwx и special указанных категорий */
    if (op == '=') {
        if (who & 1) { m &= ~0700u; m &= ~04000u; }
        if (who & 2) { m &= ~0070u; m &= ~02000u; }
        if (who & 4) { m &= ~0007u; m &= ~01000u; }
    }

    /* 4b. выставить/снять rwx (сдвиг в позицию категории) */
    unsigned rwx_u = (who & 1) ? (rwx << 6) : 0;
    unsigned rwx_g = (who & 2) ? (rwx << 3) : 0;
    unsigned rwx_o = (who & 4) ? (rwx << 0) : 0;
    unsigned rwx_all = rwx_u | rwx_g | rwx_o;

    unsigned add = 0, del = 0;
    if (op == '+')      add |= rwx_all;
    else if (op == '-') del |= rwx_all;
    else /* '=' */      add |= rwx_all;

    /* 4c. special-биты */
    unsigned sp_add = 0, sp_del = 0;
    if (set_suid) { if (op == '-') sp_del |= 04000; else sp_add |= 04000; }
    if (set_sgid) { if (op == '-') sp_del |= 02000; else sp_add |= 02000; }
    if (set_sticky) { if (op == '-') sp_del |= 01000; else sp_add |= 01000; }

    m |= add;   m |= sp_add;
    m &= ~del;  m &= ~sp_del;

    *mode = m & 07777;
    return PARSE_OK;
}

/* Применить chmod-команду (с запятыми) к base. */
static parse_status_t mode_apply_chmod_cmd(unsigned base, const char* cmd, unsigned* mode_out) {
    unsigned m = base & 07777;
    const char* p = cmd;

    while (*p) {
        const char* start = p;
        while (*p && *p != ',') ++p;
        size_t len = (size_t)(p - start);

        /* trim: убрать ведущие/завершающие пробелы */
        while (len > 0 && (*start == ' ' || *start == '\t')) { ++start; --len; }
        while (len > 0 && (start[len - 1] == ' ' || start[len - 1] == '\t')) { --len; }

        if (len == 0) return PARSE_ERR_FORMAT;   /* пустая команда */

        char buf[64];
        if (len >= sizeof buf) return PARSE_ERR_FORMAT;
        memcpy(buf, start, len);
        buf[len] = '\0';

        parse_status_t st = chmod_one(&m, buf);
        if (st != PARSE_OK) return st;

        if (*p == ',') ++p;
    }

    *mode_out = m;
    return PARSE_OK;
}

/* Диспетчер: octal -> замена маски; иначе -> chmod-команда. */
parse_status_t mode_apply_chmod(unsigned base, const char* cmd, unsigned* mode_out) {
    if (cmd == NULL || mode_out == NULL) return PARSE_ERR_FORMAT;
    if (*cmd == '\0') return PARSE_ERR_FORMAT;

    /* octal: замена маски целиком (base игнорируется, как chmod 755 file) */
    if (cmd[0] >= '0' && cmd[0] <= '9') {
        return mode_from_octal(cmd, mode_out);
    }

    /* chmod-команда: модификация base */
    return mode_apply_chmod_cmd(base, cmd, mode_out);
}