#ifndef UDP_RAW_H
#define UDP_RAW_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#define UDP_HDR_LEN 8

struct udp_datagram {
    uint32_t src_ip;        
    uint16_t src_port;      
    uint16_t dst_port;      
    const uint8_t* payload; 
    size_t   payload_len;   
};

/* Открыть raw UDP-сокет  */
int raw_open(void);

/* Принять один пакет, адресованный на want_port.
 *   1  — наш пакет, *out заполнен (payload указывает в buf)
 *   0  — не наш / короткий / не UDP / пустой payload
 *  -2  — прервано сигналом (EINTR)
 *  -1  — ошибка recvfrom */
int raw_recv_for_port(int fd, uint16_t want_port,
    uint8_t* buf, size_t buflen,
    struct udp_datagram* out);

/* Отправить UDP-дейтаграмму через raw-сокет.
 * Собирает UDP-заголовок + checksum (псевдозаголовок), IP строит ядро.
 * src_ip/src_port нужны для checksum и порта-источника.
 * payload включает код в [0]. Возвращает число байт или -1. */
ssize_t raw_send_udp(int fd,
    uint32_t src_ip, uint16_t src_port,
    uint32_t dst_ip, uint16_t dst_port,
    const void* payload, size_t len);

#endif /* UDP_RAW_H */