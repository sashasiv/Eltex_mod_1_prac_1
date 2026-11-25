#ifndef FILTER_H
#define FILTER_H

#include "chat.h"

/* Проверяет, соответствует ли пакет выбранному фильтру.
 * Возврат: 1 - пакет подходит, 0 - пакет нужно отбросить. */
int apply_filter(const packet_info_t* pkt, int filter_type);

#endif