#ifndef SNIFF_PARSE_H
#define SNIFF_PARSE_H

#include "chat.h"
#include <stddef.h>

/* Разбирает сырой Ethernet-пакет (buffer, length) в структуру packet_info_t.
 * Возврат:
 *   0  - успех (это IPv4 пакет, поля заполнены)
 *  -1  - это не IPv4 (например, ARP или IPv6), пакет проигнорирован */
int parse_packet(const unsigned char* buffer, size_t length, packet_info_t* out);

#endif