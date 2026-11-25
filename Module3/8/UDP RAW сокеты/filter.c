/* ============================================================
 *  filter.c — Фильтрация перехваченных пакетов.
 * ============================================================ */

#include "filter.h"

int apply_filter(const packet_info_t* pkt, int filter_type)
{
    switch (filter_type) {
    case FILTER_CHAT:
        /* Фильтр для Заданий 6-7.
         * Мы ищем пакеты, где порт отправителя или получателя 5000 или 8080. */
        if (pkt->protocol == 17) { /* UDP */
            if (pkt->src_port == 5000 || pkt->dst_port == 5000 ||
                pkt->src_port == 8080 || pkt->dst_port == 8080) {
                return 1;
            }
        }
        return 0;

    case FILTER_DNS:
        /* Фильтр для DNS (порт 53) */
        if (pkt->protocol == 17) {
            if (pkt->src_port == 53 || pkt->dst_port == 53) {
                return 1;
            }
        }
        return 0;

    case FILTER_ALL:
        /* Пропускаем все UDP-пакеты */
        if (pkt->protocol == 17) {
            return 1;
        }
        return 0;

    default:
        /* Неизвестный фильтр — ничего не пропускаем */
        return 0;
    }
}