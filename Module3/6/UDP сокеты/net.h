#ifndef NET_H
#define NET_H

/* ============================================================
 *  net.h — сетевой слой: создание сокета, bind, broadcast, send.
 * ============================================================ */

#include "chat.h"

 /* Создать UDP-сокет с опциями SO_BROADCAST / SO_REUSEADDR.
  * Возврат: дескриптор сокета или -1 при ошибке. */
int net_create_socket(void);

/* Привязать сокет к INADDR_ANY:port. Возврат: 0 или -1. */
int net_bind(int sock, int port);

/* Заполнить sockaddr_in broadcast-адреса из строки "a.b.c.d".
 * Возврат: 0 или -1 при неверном IP. */
int net_make_bcast(const char* ip, int port, struct sockaddr_in* out);

/* Отправить датаграмму на адрес dst.
 * Возврат: число отправленных байт или -1. */
int net_send(int sock, const struct sockaddr_in* dst,
    const char* buf, size_t len);

#endif /* NET_H */