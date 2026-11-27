#include "common.h"
#include "udp_raw.h"
#include "signals.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <arpa/inet.h>
#include <fcntl.h>

struct client_ctx {
    int      raw_fd;
    uint32_t my_ip;           /* host order */
    uint16_t my_port;         /* host order */
    uint32_t srv_ip;
    uint16_t srv_port;
};

/* ---------- отправка одного сообщения ---------- */

static void send_ping(struct client_ctx* ctx, const char* body, size_t len)
{
    uint8_t msg[MAX_PAYLOAD];
    if (len + 1 > sizeof(msg))
        len = sizeof(msg) - 1;

    msg[0] = MSG_PING;
    memcpy(msg + 1, body, len);

    if (raw_send_udp(ctx->raw_fd,
        ctx->my_ip, ctx->my_port,
        ctx->srv_ip, ctx->srv_port,
        msg, len + 1) < 0)
        fprintf(stderr, "sendto: %s\n", strerror(errno));
}

static void send_close(struct client_ctx* ctx)
{
    uint8_t msg[1] = { MSG_CLOSE };
    if (raw_send_udp(ctx->raw_fd,
        ctx->my_ip, ctx->my_port,
        ctx->srv_ip, ctx->srv_port,
        msg, sizeof(msg)) < 0)
        fprintf(stderr, "sendto CLOSE: %s\n", strerror(errno));
}

/* ---------- приём ответа ---------- */

/* Читаем всё, что пришло на наш порт, и печатаем только ответы сервера. */
static void drain_replies(struct client_ctx* ctx)
{
    uint8_t buf[65536];
    struct udp_datagram d;

    for (;;) {
        int r = raw_recv_for_port(ctx->raw_fd, ctx->my_port,
            buf, sizeof(buf), &d);

        if (r == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return; /* Пакеты кончились, выходим без блокировки */
            }
            die("recvfrom");
        }
        if (r == -2) return;              /* EINTR */
        if (r == 0) continue;             /* не наш порт */

        /* фильтр «это точно ответ нашего сервера» */
        if (d.src_ip != ctx->srv_ip || d.src_port != ctx->srv_port)
            continue;
        if (d.payload_len < 1 || d.payload[0] != MSG_PING)
            continue;

        fwrite(d.payload + 1, 1, d.payload_len - 1, stdout);
        fputc('\n', stdout);
        fflush(stdout);
    }
}

/* ---------- главный цикл ---------- */

static void run_client(struct client_ctx* ctx)
{
    struct pollfd fds[2] = {
        { .fd = STDIN_FILENO, .events = POLLIN },
        { .fd = ctx->raw_fd,  .events = POLLIN },
    };

    char line[MAX_PAYLOAD];

    while (!g_stop) {
        int pr = poll(fds, 2, -1);
        if (pr < 0) {
            if (errno == EINTR) continue;   /* сигнал → проверим g_stop */
            die("poll");
        }

        /* stdin: прочитали строку — шлём PING */
        if (fds[0].revents & POLLIN) {
            if (!fgets(line, sizeof(line), stdin)) {
                g_stop = 1;                 /* EOF (Ctrl+D) — как выход */
                break;
            }
            size_t len = strlen(line);
            while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
                line[--len] = '\0';
            if (len == 0)
                continue;                   /* пустая строка — пропуск */
            send_ping(ctx, line, len);
        }

        /* raw_fd: пришёл ответ — печатаем */
        if (fds[1].revents & POLLIN)
            drain_replies(ctx);
    }
}

int main(int argc, char** argv)
{
    struct client_ctx ctx = {
        .raw_fd = -1,
        .my_ip = 0,
        .my_port = DEFAULT_CLIENT_PORT,
        .srv_ip = 0,
        .srv_port = DEFAULT_SERVER_PORT,
    };

    if (parse_ipv4(DEFAULT_SERVER_IP, &ctx.srv_ip) < 0)
        die("parse default server ip");

    int opt;
    while ((opt = getopt(argc, argv, "s:p:c:h")) != -1) {
        switch (opt) {
        case 's':
            if (parse_ipv4(optarg, &ctx.srv_ip) < 0) die_usage(argv[0]);
            break;
        case 'p':
            if (parse_port(optarg, &ctx.srv_port) < 0) die_usage(argv[0]);
            break;
        case 'c':
            if (parse_port(optarg, &ctx.my_port) < 0) die_usage(argv[0]);
            break;
        default:
            die_usage(argv[0]);
        }
    }

    if (get_local_ip(ctx.srv_ip, &ctx.my_ip) < 0)
        die("get_local_ip");

    install_signal_handlers();

    ctx.raw_fd = raw_open();
    if (ctx.raw_fd < 0)
        die("raw_open (нужен root/CAP_NET_RAW)");

    int flags = fcntl(ctx.raw_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(ctx.raw_fd, F_SETFL, flags | O_NONBLOCK) < 0)
        die("fcntl O_NONBLOCK");

    printf("[client] %u.%u.%u.%u:%u -> %u.%u.%u.%u:%u\n",
        (ctx.my_ip >> 24) & 0xff, (ctx.my_ip >> 16) & 0xff,
        (ctx.my_ip >> 8) & 0xff, ctx.my_ip & 0xff, ctx.my_port,
        (ctx.srv_ip >> 24) & 0xff, (ctx.srv_ip >> 16) & 0xff,
        (ctx.srv_ip >> 8) & 0xff, ctx.srv_ip & 0xff, ctx.srv_port);
    fflush(stdout);

    run_client(&ctx);

    /* штатное выключение: сообщить серверу и закрыть */
    send_close(&ctx);
    printf("[client] closing\n");
    close(ctx.raw_fd);
    return 0;
}