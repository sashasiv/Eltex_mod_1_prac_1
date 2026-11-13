// parent.c
#include "parent.h"
#include "msg.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Ждёт сообщение заданного типа. 0 — успех, -1 — ошибка/EOF/не тот тип. */
static int wait_for(int rfd, int expected_type, struct msg* m) {
    int rc = recv_msg(rfd, m);
    if (rc == 0) {
        log_msg("parent", "потомок закрыл канал, ожидалось %s",
            msg_type_name(expected_type));
        return -1;
    }
    if (rc < 0) {
        log_msg("parent", "ошибка приёма в ожидании %s: %s",
            msg_type_name(expected_type), strerror(errno));
        return -1;
    }
    if (m->type != expected_type) {
        log_msg("parent", "ожидалось %s, получено %s",
            msg_type_name(expected_type), msg_type_name(m->type));
        return -1;
    }
    return 0;
}

/* Проверить и открыть входной файл. fd или -1). */
static int open_input(const char* path, off_t* size_out) {
    struct stat st;
    if (stat(path, &st) < 0) {
        fprintf(stderr, "родитель: %s: %s\n", path, strerror(errno));
        return -1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "родитель: %s: не обычный файл\n", path);
        return -1;
    }
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "родитель: %s: %s\n", path, strerror(errno));
        return -1;
    }
    *size_out = st.st_size;
    return fd;
}

int parent_copy_file(int wfd, int rfd, const char* path) {
    struct msg m;

    /* 1. открываем файл */
    off_t size = 0;
    int in = open_input(path, &size);
    if (in < 0)
        return -1;

    /* 2. ждём READY */
    if (wait_for(rfd, MSG_READY, &m) < 0) {
        close(in);
        return -1;
    }
    log_msg("parent", "получено READY для '%s'", path);

    /* 3. HEADER */
    if (size > INT32_MAX) {
        log_msg("parent", "'%s' слишком велик для протокола (%lld байт)",
            path, (long long)size);
        close(in);
        return -1;
    }
    memset(&m, 0, sizeof(m));
    m.type = MSG_HEADER;
    m.size = (int32_t)size;
    strncpy(m.payload, path, BLOCK_SIZE - 1);
    m.payload[BLOCK_SIZE - 1] = '\0';
    if (send_msg(wfd, &m) < 0) {
        log_msg("parent", "не удалось отправить HEADER: %s", strerror(errno));
        close(in);
        return -1;
    }
    log_msg("parent", "отправлен HEADER '%s' размер=%lld",
        path, (long long)size);

    /* 4. DATA */
    ssize_t r;
    while ((r = read(in, m.payload, BLOCK_SIZE)) > 0) {
        m.type = MSG_DATA;
        m.size = (int32_t)r;
        if (send_msg(wfd, &m) < 0) {
            log_msg("parent", "не удалось отправить DATA: %s", strerror(errno));
            close(in);
            return -1;
        }
        log_msg("parent", "отправлено DATA %zd байт", r);
    }
    if (r < 0) {
        log_msg("parent", "чтение '%s': %s", path, strerror(errno));
        close(in);
        return -1;
    }
    if (close(in) < 0)
        log_msg("parent", "закрытие '%s': %s", path, strerror(errno));

    /* 5. DONE */
    if (wait_for(rfd, MSG_DONE, &m) < 0) {
        log_msg("parent", "не получено DONE для '%s'", path);
        return -1;
    }
    log_msg("parent", "получено DONE для '%s'", path);
    return 0;
}

int parent_send_end(int wfd) {
    struct msg m;
    memset(&m, 0, sizeof(m));
    m.type = MSG_END;
    if (send_msg(wfd, &m) < 0) {
        log_msg("parent", "не удалось отправить END: %s", strerror(errno));
        return -1;
    }
    log_msg("parent", "отправлен END");
    return 0;
}