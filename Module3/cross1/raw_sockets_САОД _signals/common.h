#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <netinet/in.h>

/* ---------- конфиг ---------- */
#define DEFAULT_SERVER_IP    "192.168.10.1"
#define DEFAULT_SERVER_PORT  5555u
#define DEFAULT_CLIENT_PORT  6001u

/* Коды сообщений: первый байт payload */
#define MSG_PING   0x00
#define MSG_CLOSE  0x01

/* Ограничение payload (без IP/UDP заголовков), чтобы не фрагментировать */
#define MAX_PAYLOAD 1400

/* ---------- утилиты ---------- */

/* die("msg") — печатает в stderr с errno и exit(1) */
void die(const char* msg) __attribute__((noreturn));

/* die_usage("prog") — короткая подсказка и exit(2) */
void die_usage(const char* prog) __attribute__((noreturn));

/* Парсинг IPv4 из строки. Возвращает 0 успех, -1 ошибка. host order */
int parse_ipv4(const char* s, uint32_t* out);

/* Парсинг порта 1..65535. Возвращает 0 успех, -1 ошибка. host order */
int parse_port(const char* s, uint16_t* out);

/* Узнать локальный IP, который будет использован для отправки на dst_ip.
 * Возвращает 0 успех, -1 ошибка. host order. */
int get_local_ip(uint32_t dst_ip, uint32_t* out_ip);

#endif /* COMMON_H */