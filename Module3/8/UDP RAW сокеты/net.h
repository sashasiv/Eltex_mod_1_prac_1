#ifndef NET_H
#define NET_H

#include "chat.h"

/* Создает RAW-сокет и привязывает его к указанному интерфейсу.
 * Возврат: дескриптор сокета или -1 при ошибке. */
int raw_socket_create(const char* ifname);

#endif /* NET_H */