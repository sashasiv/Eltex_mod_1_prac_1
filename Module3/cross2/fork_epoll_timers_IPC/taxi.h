#ifndef TAXI_H
#define TAXI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#define MAX_DRIVERS 64

enum { CMD_SEND_TASK = 1, CMD_GET_STATUS = 2 };
enum { ST_AVAILABLE = 0, ST_BUSY = 1 };

struct msg {
    int type;       /* 0 = CMD, 1 = RESP */
    int cmd;
    int task_timer;
    int status;
    int remaining;
    char text[64];
};

static inline void set_nonblock(int fd) {
    int fl = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

/* Полный read/write для фиксированного размера сообщения (с EINTR) */
static inline int full_write(int fd, const void* buf, size_t n) {
    const char* p = buf;
    while (n) {
        ssize_t r = write(fd, p, n);
        if (r < 0) { 
            if (errno == EINTR) continue; 
            return -1; 
        }
        p += r; n -= r;
    }
    return 0;
}

#endif