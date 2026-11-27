#include "common.h"
#include "udp_raw.h"
#include "signals.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

/* ---------- состояние ---------- */

struct client_entry {
    uint32_t ip;              /* host order */
    uint16_t port;            /* host order */
    uint32_t counter;
    struct client_entry* next;
};

struct server_ctx {
    int      raw_fd;
    uint32_t listen_ip;       /* src_ip для ответов (checksum) */
    uint16_t listen_port;
    struct client_entry* clients;
};

/* ---------- список клиентов ---------- */

static struct client_entry* find_client(struct server_ctx* ctx,
    uint32_t ip, uint16_t port)
{
    for (struct client_entry* e = ctx->clients; e; e = e->next)
        if (e->ip == ip && e->port == port)
            return e;
    return NULL;
}

static struct client_entry* get_or_create_client(struct server_ctx* ctx,
    uint32_t ip, uint16_t port)
{
    struct client_entry* e = find_client(ctx, ip, port);
    if (e)
        return e;

    e = calloc(1, sizeof(*e));
    if (!e)
        die("calloc client_entry");
    e->ip = ip;
    e->port = port;
    e->counter = 0;
    e->next = ctx->clients;
    ctx->clients = e;
    return e;
}

static void remove_client(struct server_ctx* ctx, uint32_t ip, uint16_t port)
{
    struct client_entry** pp = &ctx->clients;
    while (*pp) {
        struct client_entry* e = *pp;
        if (e->ip == ip && e->port == port) {
            *pp = e->next;
            free(e);
            return;
        }
        pp = &e->next;
    }
    /* не найден — не ошибка (идемпотентно) */
}

static void free_clients(struct server_ctx* ctx)
{
    struct client_entry* e = ctx->clients;
    while (e) {
        struct client_entry* n = e->next;
        free(e);
        e = n;
    }
    ctx->clients = NULL;
}

/* ---------- ответ ---------- */

/* Собираем [MSG_PING][body + ' ' + N] и шлём обратно.
 * body_len — длина тела исходного PING (без кода). */
static void send_reply(struct server_ctx* ctx, const struct udp_datagram* d,
    uint32_t counter)
{
    char reply[MAX_PAYLOAD];
    size_t off = 0;

    reply[off++] = (char)MSG_PING;

    size_t body_len = d->payload_len - 1;         /* без кода */
    if (off + body_len + 16 > sizeof(reply))
        body_len = sizeof(reply) - off - 16;      /* усечение под " N" */
    memcpy(reply + off, d->payload + 1, body_len);
    off += body_len;

    int n = snprintf(reply + off, sizeof(reply) - off, " %u", counter);
    if (n < 0)
        return;
    off += (size_t)n;

    if (raw_send_udp(ctx->raw_fd,
        ctx->listen_ip, ctx->listen_port,
        d->src_ip, d->src_port,
        reply, off) < 0)
        fprintf(stderr, "sendto reply: %s\n", strerror(errno));
}
/* ---------- отладка ---------- */

static void print_ip(uint32_t ip)
{
    struct in_addr a = { .s_addr = htonl(ip) };
    char s[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &a, s, sizeof(s));
    fputs(s, stdout);
}

static void debug_print_datagram(const struct udp_datagram* d)
{
    printf("[debug] recv from ");
    print_ip(d->src_ip);
    printf(":%u -> port %u | len=%zu | code=0x%02x | body=\"",
        d->src_port, d->dst_port, d->payload_len, d->payload[0]);

    /* Печатаем тело, заменяя непечатаемые символы */
    for (size_t i = 1; i < d->payload_len; ++i) {
        char c = (char)d->payload[i];
        if (c >= 32 && c <= 126) {
            putchar(c);
        }
        else if (c == '\n') {
            fputs("\\n", stdout);
        }
        else if (c == '\r') {
            fputs("\\r", stdout);
        }
        else {
            printf("\\x%02x", (unsigned char)c);
        }
    }
    printf("\"\n");
    fflush(stdout);
}

static void handle_datagram(struct server_ctx* ctx,
    const struct udp_datagram* d)
{
    /* Выводим отладочную информацию для каждого валидного пакета */
    debug_print_datagram(d);

    uint8_t code = d->payload[0];

    switch (code) {
    case MSG_PING: {
        struct client_entry* e =
            get_or_create_client(ctx, d->src_ip, d->src_port);
        e->counter++;
        send_reply(ctx, d, e->counter);
        break;
    }
    case MSG_CLOSE:
        printf("[debug] client "); print_ip(d->src_ip); printf(":%u closed\n", d->src_port);
        remove_client(ctx, d->src_ip, d->src_port);
        break;
    default:
        printf("[debug] unknown code 0x%02x, ignoring\n", code);
        break;
    }
}

/* ---------- главный цикл ---------- */

static void run_server(struct server_ctx* ctx)
{
    uint8_t buf[65536];
    struct udp_datagram d;

    while (!g_stop) {
        int r = raw_recv_for_port(ctx->raw_fd, ctx->listen_port,
            buf, sizeof(buf), &d);
        if (r == -2) continue;            /* EINTR */
        if (r == -1) die("recvfrom");
        if (r == 0) continue;             /* не наш */

        handle_datagram(ctx, &d);
    }
}

int main(int argc, char** argv)
{
    struct server_ctx ctx = {
        .raw_fd = -1,
        .listen_ip = 0,
        .listen_port = DEFAULT_SERVER_PORT,
        .clients = NULL,
    };

    int opt;
    while ((opt = getopt(argc, argv, "p:h")) != -1) {
        switch (opt) {
        case 'p':
            if (parse_port(optarg, &ctx.listen_port) < 0)
                die_usage(argv[0]);
            break;
        default:
            die_usage(argv[0]);
        }
    }

    if (parse_ipv4(DEFAULT_SERVER_IP, &ctx.listen_ip) < 0)
        die("parse default server ip");

    install_signal_handlers();

    ctx.raw_fd = raw_open();
    if (ctx.raw_fd < 0)
        die("raw_open (нужен root/CAP_NET_RAW)");

    printf("[server] raw UDP listening on %s:%u\n",
        DEFAULT_SERVER_IP, ctx.listen_port);
    fflush(stdout);

    run_server(&ctx);

    printf("[server] shutting down\n");
    close(ctx.raw_fd);
    free_clients(&ctx);
    return 0;
}