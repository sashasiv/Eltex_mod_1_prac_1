#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

/* Инициализация генератора (затравка текущим временем) */
void random_init(void);

/* Сгенерировать случайный IPv4-адрес (32-битное число) */
uint32_t random_ipv4(void);

#endif /* RANDOM_H */
