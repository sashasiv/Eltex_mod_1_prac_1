// util.h
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <sys/types.h>

#include "msg.h"

/* ---------- небуферизованный I/O ---------- */

/* Прочитать ровно n байт в buf.
   Возвращает:
     n     — успех (прочитано ровно n);
     0..n-1 — короткое чтение (EOF посреди);
     -1    — ошибка (errno установлен). */
ssize_t read_full(int fd, void* buf, size_t n);

/* Записать ровно n байт из buf.
   Возвращает n при успехе, < n при короткой записи, -1 при ошибке. */
ssize_t write_full(int fd, const void* buf, size_t n);

/* ---------- сообщения протокола ---------- */

/* Отправить одно сообщение целиком.
   Возвращает 0 при успехе, -1 при ошибке. */
int send_msg(int fd, const struct msg* m);

/* Принять одно сообщение целиком.
   Возвращает:
     1  — получено полное сообщение;
     0  — EOF (пишущая сторона закрыла канал);
     -1 — ошибка 
     -2 - неполное сообщение. */
int recv_msg(int fd, struct msg* m);

/* ---------- логирование ---------- */

/* Пишет в stderr строку вида:
     [role pid=NNNN] <сообщение>\n
   Роль — короткая строка ("main", "parent", "child"). */
void log_msg(const char* role, const char* fmt, ...)
__attribute__((format(printf, 2, 3)));

#endif