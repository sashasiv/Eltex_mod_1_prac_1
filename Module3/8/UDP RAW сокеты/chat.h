#ifndef SNIFFER_H
#define SNIFFER_H

#include <stddef.h>

#define MAC_STR_LEN 18
#define IP_STR_LEN  16

/* Типы фильтров */
#define FILTER_CHAT 1
#define FILTER_DNS  2
#define FILTER_ALL  3

typedef struct {
    char src_mac[MAC_STR_LEN];
    char dst_mac[MAC_STR_LEN];
    char src_ip[IP_STR_LEN];
    char dst_ip[IP_STR_LEN];
    unsigned short src_port;
    unsigned short dst_port;
    unsigned char protocol;
    size_t payload_len;
} packet_info_t;

#endif