#include "random.h"
#include <stdlib.h>   /* rand, srand */
#include <time.h>     /* time */

void random_init(void) {
    /* Затравка текущим временем, чтобы числа были разными при каждом запуске */
    srand((unsigned int)time(NULL));
}

uint32_t random_ipv4(void) {
    /* Генерируем 4 случайных байта (значения от 0 до 255) */
    uint32_t b1 = (uint32_t)(rand() & 0xFF); 
    uint32_t b2 = (uint32_t)(rand() & 0xFF); 
    uint32_t b3 = (uint32_t)(rand() & 0xFF); 
    uint32_t b4 = (uint32_t)(rand() & 0xFF); 

    /* Склеиваем их через побитовый сдвиг и ИЛИ */
    return (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
}