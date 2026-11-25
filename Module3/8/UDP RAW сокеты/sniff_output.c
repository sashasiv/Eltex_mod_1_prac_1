#include "sniff_output.h"
#include <stdio.h>

void print_packet_info(const packet_info_t* pkt, time_t start_time)
{
    /* Считаем время с момента запуска */
    double time_diff = difftime(time(NULL), start_time);

    /* Выводим всё в одну строку для удобства чтения */
    printf("[+%05.1f сек] ", time_diff);
    printf("MAC: %s -> %s | ", pkt->src_mac, pkt->dst_mac);
    printf("IP: %-15s -> %-15s | ", pkt->src_ip, pkt->dst_ip);
    printf("UDP: %-5u -> %-5u\n", pkt->src_port, pkt->dst_port);
}