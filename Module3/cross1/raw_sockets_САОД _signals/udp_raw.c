#include "udp_raw.h"
#include "common.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

/* ================= открытие ================= */

int raw_open(void)
{
    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (fd < 0)
        return -1;

    int off = 0;
    if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &off, sizeof(off)) < 0) {
        int saved = errno;
        close(fd);
        errno = saved;
        return -1;
    }
    return fd;
}

/* ================= приём ================= */

int raw_recv_for_port(int fd, uint16_t want_port,
    uint8_t* buf, size_t buflen,
    struct udp_datagram* out)
{
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);

    ssize_t n = recvfrom(fd, buf, buflen, 0,
        (struct sockaddr*)&from, &fromlen);
    if (n < 0) {
        if (errno == EINTR) return -2;
        return -1;
    }

    if ((size_t)n < sizeof(struct iphdr)) return 0;
    const struct iphdr* ip = (const struct iphdr*)buf;
    if (ip->version != 4) return 0;
    size_t ip_hlen = (size_t)ip->ihl * 4u;
    if (ip_hlen < sizeof(struct iphdr)) return 0;
    if (ip->protocol != IPPROTO_UDP) return 0;
    if ((size_t)n < ip_hlen + UDP_HDR_LEN) return 0;

    const struct udphdr* udp = (const struct udphdr*)(buf + ip_hlen);

    uint16_t dport = ntohs(udp->dest);
    if (dport != want_port) return 0;

    uint16_t ulen = ntohs(udp->len);
    if (ulen < UDP_HDR_LEN) return 0;

    size_t payload_len = (size_t)ulen - UDP_HDR_LEN;
    if (ip_hlen + UDP_HDR_LEN + payload_len > (size_t)n)
        payload_len = (size_t)n - ip_hlen - UDP_HDR_LEN;

    if (payload_len == 0) return 0;

    out->src_ip = ntohl(ip->saddr);
    out->src_port = ntohs(udp->source);
    out->dst_port = dport;
    out->payload = buf + ip_hlen + UDP_HDR_LEN;
    out->payload_len = payload_len;
    return 1;
}

/* ================= checksum ================= */

/* Сумма 16-битных слов в host order (после ntohs каждого слова),
 * с переносом старшего бита в младший и финальным ~. */
static uint16_t csum_fold(uint32_t sum)
{
    while (sum >> 16)
        sum = (sum & 0xffffu) + (sum >> 16);
    return (uint16_t)~sum;
}

static uint32_t csum_add_bytes(uint32_t sum, const uint8_t* p, size_t len)
{
    while (len >= 2) {
        uint16_t w;
        memcpy(&w, p, 2);
        sum += ntohs(w);          /* в сетевом порядке слово → host */
        p += 2;
        len -= 2;
    }
    if (len == 1) {
        uint16_t w = (uint16_t)p[0] << 8;  /* нечётный хвост: старший байт */
        sum += w;
    }
    return sum;
}

/* UDP checksum с псевдозаголовком (RFC 768):
 *   src_ip(4) | dst_ip(4) | 0 | proto(17) | udp_len(2)
 *   + UDP-заголовок (с check=0) + payload
 * Все IP в host order, длина — в байтах. Возвращает checksum в host order
 * (готов к htons при записи в заголовок). */
static uint16_t udp_checksum(uint32_t src_ip, uint32_t dst_ip,
    const uint8_t* udp_seg, size_t udp_len)
{
    uint32_t sum = 0;

    /* псевдозаголовок: складываем как 16-битные слова */
    sum += (src_ip >> 16) & 0xffffu;
    sum += src_ip & 0xffffu;
    sum += (dst_ip >> 16) & 0xffffu;
    sum += dst_ip & 0xffffu;
    sum += IPPROTO_UDP;                 /* 17 */
    sum += (uint32_t)udp_len;           /* длина UDP в байтах */

    /* UDP-заголовок + payload (check уже 0 в буфере) */
    sum = csum_add_bytes(sum, udp_seg, udp_len);

    uint16_t r = csum_fold(sum);
    return r == 0 ? 0xffffu : r;        /* 0 → 0xFFFF по RFC 768 */
}

/* ================= отправка ================= */

ssize_t raw_send_udp(int fd,
    uint32_t src_ip, uint16_t src_port,
    uint32_t dst_ip, uint16_t dst_port,
    const void* payload, size_t len)
{
    if (len > MAX_PAYLOAD) {
        errno = EMSGSIZE;
        return -1;
    }

    uint8_t buf[UDP_HDR_LEN + MAX_PAYLOAD];
    struct udphdr* udp = (struct udphdr*)buf;

    udp->source = htons(src_port);
    udp->dest = htons(dst_port);
    udp->len = htons((uint16_t)(UDP_HDR_LEN + len));
    udp->check = 0;
    memcpy(buf + UDP_HDR_LEN, payload, len);

    udp->check = htons(udp_checksum(src_ip, dst_ip, buf, UDP_HDR_LEN + len));

    struct sockaddr_in dst = {
        .sin_family = AF_INET,
        .sin_port = htons(dst_port),   /* для raw игнорируется, но пусть */
        .sin_addr = { .s_addr = htonl(dst_ip) },
    };

    return sendto(fd, buf, UDP_HDR_LEN + len, 0,
        (struct sockaddr*)&dst, sizeof(dst));
}