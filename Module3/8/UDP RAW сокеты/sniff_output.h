#ifndef SNIFF_OUTPUT_H
#define SNIFF_OUTPUT_H

#include "chat.h"
#include <time.h>

/* Выводит информацию о пакете на экран.
 * start_time — время старта программы для расчета разницы. */
void print_packet_info(const packet_info_t* pkt, time_t start_time);

#endif