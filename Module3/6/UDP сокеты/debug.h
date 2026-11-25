#ifndef DEBUG_H
#define DEBUG_H

/* ============================================================
 *  debug.h — отладочный режим: дамп отправленных/принятых
 *            датаграмм, hex + ASCII.
 * ============================================================ */

#include <stddef.h>
#include <netinet/in.h>

 /* Включить/выключить отладочный режим (по умолчанию выключен). */
void debug_set_enabled(int on);
int  debug_enabled(void);

/* Дамп датаграммы.
 *   dir  — "SEND" или "RECV"
 *   dst  — адрес получателя/отправителя
 *   buf  — данные, len — длина в байтах
 * Печатает: направление, IP:port, длину, hex-дамп, ASCII-дамп. */
void debug_dump(const char* dir, const struct sockaddr_in* addr,
    const void* buf, size_t len);

#endif /* DEBUG_H */