#include "core.h"
#include <stdio.h>   /* snprintf */
#include <ctype.h>   /* isdigit */
#include <string.h>

parse_ip_status_t parse_ipv4(const char* s, uint32_t* out) {
    if (s == NULL || out == NULL) return PARSE_IP_ERR_POINTER;
    if (*s == '\0') return PARSE_IP_ERR_FORMAT;

    uint32_t ip = 0;
    int octets = 0;
    const char* p = s;

    while (octets < 4) {
        /* Cтрого a.b.c.d */
        if (!isdigit((unsigned char)*p)) return PARSE_IP_ERR_FORMAT;

        int val = 0;
        while (isdigit((unsigned char)*p)) {
            val = val * 10 + (*p - '0');
            if (val > 255) return PARSE_IP_ERR_RANGE;
            ++p;
        }

        ip = (ip << 8) | (uint32_t)val;
        ++octets;

        if (*p == '.') {
            ++p;
        }
        else if (*p != '\0') {
            return PARSE_IP_ERR_FORMAT; /* мусор после числа */
        }
    }

    if (*p != '\0') return PARSE_IP_ERR_FORMAT;

    *out = ip;
    return PARSE_IP_OK;
}

bool is_valid_mask(uint32_t mask) {
    /* Маска непрерывна, если ~mask (нули слева, единицы справа) тоже непрерывна.
       Для ~mask: (~mask & (~mask + 1)) == 0, если единицы идут подряд справа. */
    uint32_t inv = ~mask;
    if (inv == 0) return true;              /* mask = 0xFFFFFFFF (все единицы) */
    if ((inv & (inv + 1)) != 0) return false; /* дырка в нулях маски */
    return true;
}

uint32_t subnet_address(uint32_t ip, uint32_t mask) {
    return ip & mask;
}

bool is_in_subnet(uint32_t ip, uint32_t network, uint32_t mask) {
    return (ip & mask) == network;
}

void format_ipv4(uint32_t ip, char out[16]) {
    snprintf(out, 16, "%u.%u.%u.%u",
        (ip >> 24) & 0xFF,
        (ip >> 16) & 0xFF,
        (ip >> 8) & 0xFF,
        ip & 0xFF);
}

const char* parse_ip_status_str(parse_ip_status_t st) {
    switch (st) {
    case PARSE_IP_OK:          return "OK";
    case PARSE_IP_ERR_FORMAT:  return "неверный формат IP (ожидается a.b.c.d)";
    case PARSE_IP_ERR_RANGE:   return "число в IP вне диапазона 0..255";
    case PARSE_IP_ERR_MASK:    return "маска не является непрерывной";
    case PARSE_IP_ERR_POINTER: return "ошибка в указателях";
    default:                   return "неизвестная ошибка";
    }
}