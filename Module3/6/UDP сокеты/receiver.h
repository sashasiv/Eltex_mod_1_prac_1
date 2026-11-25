#ifndef RECEIVER_H
#define RECEIVER_H

/* ============================================================
 *  receiver.h — поток приёма UDP-датаграмм.
 * ============================================================ */

#include "chat.h"

 /* Точка входа потока. arg = client_t*.
  * Возврат: NULL. Завершается, когда c->running == 0. */
void* receiver_thread(void* arg);

/* Попросить поток приёма завершиться и разблокировать recvfrom.
 * Вызывается из главного потока. */
void receiver_stop(client_t* c);

/* Напечатать строку в консоль под мьютексом контекста.
 * Используется и receiver, и main (для приглашения). */
void receiver_print(client_t* c, const char* fmt, ...);

#endif /* RECEIVER_H */