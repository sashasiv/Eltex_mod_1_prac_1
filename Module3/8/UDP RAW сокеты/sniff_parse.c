/* ============================================================
 *  sniff_parse.c — Разбор сырых Ethernet/IP/UDP пакетов.
 * ============================================================ */

#include "sniff_parse.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

int parse_packet(const unsigned char* buffer, size_t length, packet_info_t* out)
{
    /* 1. Проверяем, что пакет хотя бы такого же размера, как Ethernet-заголовок */
    if (length < sizeof(struct ethhdr)) return -1;

    const struct ethhdr* eth = (const struct ethhdr*)buffer;

    /* 2. Нас интересует только IPv4 (EtherType = 0x0800) */
    if (ntohs(eth->h_proto) != 0x0800) return -1;

    /* 3. Проверяем, что пакет содержит полный IP-заголовок */
    if (length < sizeof(struct ethhdr) + sizeof(struct iphdr)) return -1;

    const struct iphdr* ip = (const struct iphdr*)(buffer + sizeof(struct ethhdr));

    /* Очищаем структуру перед заполнением */
    memset(out, 0, sizeof(*out));

    /* 4. Извлекаем MAC-адреса и переводим их в строку "xx:xx:xx:xx:xx:xx" */
    snprintf(out->src_mac, MAC_STR_LEN, "%02x:%02x:%02x:%02x:%02x:%02x",
        eth->h_source[0], eth->h_source[1], eth->h_source[2],
        eth->h_source[3], eth->h_source[4], eth->h_source[5]);

    snprintf(out->dst_mac, MAC_STR_LEN, "%02x:%02x:%02x:%02x:%02x:%02x",
        eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
        eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);

    /* 5. Извлекаем IP-адреса (они в сетевом порядке, используем inet_ntoa) */
    struct in_addr saddr, daddr;
    saddr.s_addr = ip->saddr;
    daddr.s_addr = ip->daddr;

    strncpy(out->src_ip, inet_ntoa(saddr), IP_STR_LEN - 1);
    strncpy(out->dst_ip, inet_ntoa(daddr), IP_STR_LEN - 1);

    out->protocol = ip->protocol;

    /* 6. Если протокол UDP (17), извлекаем порты */
    if (ip->protocol == 17) {
        /* Внимание: IP-заголовок может иметь опции, поэтому его длина = ihl */
        int ip_hdr_len = ip->ihl * 4;

        /* Проверяем, что пакет достаточно длинный для UDP-заголовка */
        if (length < sizeof(struct ethhdr) + ip_hdr_len + sizeof(struct udphdr)) {
            return -1;
        }

        const struct udphdr* udp = (const struct udphdr*)((const unsigned char*)ip + ip_hdr_len);

        out->src_port = ntohs(udp->source);
        out->dst_port = ntohs(udp->dest);
        out->payload_len = ntohs(udp->len) - sizeof(struct udphdr);
    }

    return 0;
}