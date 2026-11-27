#include "taxi.h"

static int tfd = -1;
static int busy = 0;
static int task_seconds = 0;
static struct timespec task_start;

static void start_timer(int sec) {
    struct itimerspec its = { 0 };
    its.it_value.tv_sec = sec;
    timerfd_settime(tfd, 0, &its, NULL);
    clock_gettime(CLOCK_MONOTONIC, &task_start);
    task_seconds = sec;
    busy = 1;
}

static int remaining(void) {
    if (!busy) return 0;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double el = (now.tv_sec - task_start.tv_sec)
        + (now.tv_nsec - task_start.tv_nsec) / 1e9;
    int r = task_seconds - (int)el;
    return r > 0 ? r : 0;
}

static void handle(const struct msg* in, struct msg* out, int resp_fd) {
    memset(out, 0, sizeof(*out));
    out->type = 1;
    out->cmd = in->cmd;

    if (in->cmd == CMD_SEND_TASK) {
        if (busy) {
            out->status = ST_BUSY;
            out->remaining = remaining();
            snprintf(out->text, sizeof(out->text), "Занят ещё %d сек.", out->remaining);
        }
        else {
            start_timer(in->task_timer);
            out->status = ST_AVAILABLE;
            snprintf(out->text, sizeof(out->text),
                "Задача принята осталось %d сек.", in->task_timer);
        }
    }
    else if (in->cmd == CMD_GET_STATUS) {
        if (busy) {
            out->status = ST_BUSY;
            out->remaining = remaining();
            snprintf(out->text, sizeof(out->text), "Занят ещё %d сек.", out->remaining);
        }
        else {
            out->status = ST_AVAILABLE;
            snprintf(out->text, sizeof(out->text), "Свободен");
        }
    }
    else {
        snprintf(out->text, sizeof(out->text), "Неизвестная команда");
    }

    full_write(resp_fd, out, sizeof(*out));
}

int main(int argc, char** argv) {
    if (argc != 3) { fprintf(stderr, "использование: driver <cmd_r> <resp_w>\n"); return 1; }
    int cmd_r = atoi(argv[1]);
    int resp_w = atoi(argv[2]);

    set_nonblock(cmd_r);
    set_nonblock(resp_w);

    tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (tfd < 0) { perror("timerfd_create"); return 1; }

    int ep = epoll_create1(0);
    struct epoll_event ev, events[2];

    ev.events = EPOLLIN; ev.data.fd = cmd_r;
    epoll_ctl(ep, EPOLL_CTL_ADD, cmd_r, &ev);
    ev.events = EPOLLIN; ev.data.fd = tfd;
    epoll_ctl(ep, EPOLL_CTL_ADD, tfd, &ev);

    fprintf(stderr, "[driver %d] started\n", getpid());

    for (;;) {
        int n = epoll_wait(ep, events, 2, -1);
        if (n < 0) { if (errno == EINTR) continue; break; }

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == tfd) {
                uint64_t exp;
                read(tfd, &exp, sizeof(exp));
                busy = 0;
                task_seconds = 0;
                fprintf(stderr, "[Водитель %d] Задача выполнена -> Готов к новой\n", getpid());
            }
            else {
                struct msg in, out;
                ssize_t r = read(cmd_r, &in, sizeof(in));
                if (r == 0) {           /* родитель закрыл pipe — выходим */
                    close(ep); close(cmd_r); close(resp_w); close(tfd);
                    return 0;
                }
                if (r == (ssize_t)sizeof(in))
                    handle(&in, &out, resp_w);
            }
        }
    }
    close(ep); close(cmd_r); close(resp_w); close(tfd);
    return 0;
}