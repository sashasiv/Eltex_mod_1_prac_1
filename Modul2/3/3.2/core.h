#ifndef CORE_H
#define CORE_H

#include <stdint.h>
#include <stdbool.h>

/* Результат парсинга */
typedef enum {
    PARSE_IP_OK = 0,
    PARSE_IP_ERR_FORMAT,   /* не a.b.c.d */
    PARSE_IP_ERR_RANGE,    /* число > 255 или < 0 */
    PARSE_IP_ERR_MASK,      /* маска не непрерывна */
    PARSE_IP_ERR_POINTER
} parse_ip_status_t;

/* Парсинг IP-строки "a.b.c.d" в uint32_t.
   Старший байт = a, младший = d. */
parse_ip_status_t parse_ipv4(const char* s, uint32_t* out);

/* Проверка, что маска непрерывна (единицы слева, нули справа). */
bool is_valid_mask(uint32_t mask);

/* Вычисление адреса сети: ip & mask */
uint32_t subnet_address(uint32_t ip, uint32_t mask);

/* Принадлежность IP сети (true = в сети, false = вне). */
bool is_in_subnet(uint32_t ip, uint32_t network, uint32_t mask);

/* Форматирование uint32_t -> строка "a.b.c.d". Буфер минимум 16 байт. */
void format_ipv4(uint32_t ip, char out[16]);

/* Строка для кода ошибки парсинга */
const char* parse_ip_status_str(parse_ip_status_t st);

#endif /* CORE_H */
