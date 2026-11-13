// child.c
#include "child.h"
#include "msg.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Открыть <in_name>.copy на запись (создать/усечь).
   Возвращает fd или -1 (с диагностикой в stderr). */
static int open_out(const char* in_name) {
    char outname[BLOCK_SIZE + 8];
    snprintf(outname, sizeof(outname), "%s.copy", in_name);

    int fd = open(outname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        fprintf(stderr, "потомок: %s: %s\n", outname, strerror(errno));
    else
        log_msg("child", "открыт %s", outname);
    return fd;
}

void child_run(int rfd, int wfd) {
    struct msg m;

    for (;;) {
        /* ---------- 1. сообщаем о готовности ---------- */
        memset(&m, 0, sizeof(m));
        m.type = MSG_READY;
        if (send_msg(wfd, &m) < 0) {
            log_msg("child", "не удалось отправить READY: %s", strerror(errno));
            _exit(EXIT_FAILURE);
        }
        log_msg("child", "отправлен READY");

        /* ---------- 2. ждём HEADER или END ---------- */
        int rc = recv_msg(rfd, &m);
        if (rc == 0) {
            log_msg("child", "родитель закрыл канал (EOF), выход");
            _exit(EXIT_SUCCESS);
        }
        if (rc < 0) {
            log_msg("child", "ошибка приёма: %s", strerror(errno));
            _exit(EXIT_FAILURE);
        }
        log_msg("child", "получено %s", msg_type_name(m.type));

        /* ---------- 3. разбираем тип ---------- */
        if (m.type == MSG_END) {
            log_msg("child", "получен END, выход");
            _exit(EXIT_SUCCESS);
        }
        if (m.type != MSG_HEADER) {
            log_msg("child", "неожиданное сообщение %s", msg_type_name(m.type));
            _exit(EXIT_FAILURE);
        }

        /* ---------- 4. открываем выходной файл ----------
           Имя копируем СРАЗУ: m.payload будет перезаписан
           первым же DATA, а имя понадобится в логе после close. */
        char cur_name[BLOCK_SIZE];
        snprintf(cur_name, sizeof(cur_name), "%s", m.payload);

        int out = open_out(cur_name);
        int remaining = m.size;
        log_msg("child", "начат файл '%s' размер=%d", cur_name, remaining);

        /* ---------- 5. принимаем DATA ровно size байт ---------- */
        while (remaining > 0) {
            rc = recv_msg(rfd, &m);
            if (rc == 0) {
                log_msg("child", "связь потеряна в DATA (EOF)");
                if (out >= 0) close(out);
                _exit(EXIT_FAILURE);
            }
            if (rc < 0) {
                log_msg("child", "ошибка приёма DATA: %s", strerror(errno));
                if (out >= 0) close(out);
                _exit(EXIT_FAILURE);
            }
            if (m.type != MSG_DATA) {
                log_msg("child", "некорректный DATA (type=%s size=%d)",
                    msg_type_name(m.type), m.size);
                if (out >= 0) close(out);
                _exit(EXIT_FAILURE);
            }
            if (out >= 0) {
                if (write_full(out, m.payload, (size_t)m.size) < 0) {
                    log_msg("child", "запись: %s", strerror(errno));
                    close(out);
                    out = -1;
                }
            }
            remaining -= m.size;
            log_msg("child", "блок %d байт, осталось %d", m.size, remaining);
        }

        /* ---------- 6. закрываем выход ---------- */
        if (out >= 0) {
            if (close(out) < 0)
                log_msg("child", "закрытие '%s.copy': %s",
                    cur_name, strerror(errno));
            else
                log_msg("child", "закрыт выход для '%s'", cur_name);
        }

        /* ---------- 7. подтверждаем ---------- */
        memset(&m, 0, sizeof(m));
        m.type = MSG_DONE;
        if (send_msg(wfd, &m) < 0) {
            log_msg("child", "не удалось отправить DONE: %s", strerror(errno));
            _exit(EXIT_FAILURE);
        }
        log_msg("child", "отправлен DONE");
    }
}