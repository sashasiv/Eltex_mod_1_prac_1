// main.c
#include "child.h"
#include "parent.h"
#include "msg.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* ---------- вспомогательное ---------- */

static void die(const char* what) {
    fprintf(stderr, "main: %s: %s\n", what, strerror(errno));
    exit(EXIT_FAILURE);
}

static void usage(const char* prog) {
    fprintf(stderr, "использование: %s [-p БАЗА] файл1 [файл2 ...]\n", prog);
    exit(EXIT_FAILURE);
}

/* ---------- анонимные каналы ---------- */

static int run_anon(char** files, int nfiles) {
    int p2c[2];   /* parent -> child */
    int c2p[2];   /* child  -> parent */

    if (pipe(p2c) < 0) die("pipe p2c");
    if (pipe(c2p) < 0) die("pipe c2p");

    log_msg("main", "созданы анонимные каналы: p2c=[%d,%d] c2p=[%d,%d]",
        p2c[0], p2c[1], c2p[0], c2p[1]);

    pid_t pid = fork();
    if (pid < 0) die("fork");

    if (pid == 0) {
        /* ребёнок: p2c[0] — читает, c2p[1] — пишет */
        close(p2c[1]);
        close(c2p[0]);
        log_msg("child", "запуск (анонимные), rfd=%d wfd=%d", p2c[0], c2p[1]);
        child_run(p2c[0], c2p[1]);
        _exit(EXIT_FAILURE);   /* недостижимо */
    }

    /* родитель: p2c[1] — пишет, c2p[0] — читает */
    close(p2c[0]);
    close(c2p[1]);
    log_msg("parent", "запуск (анонимные), wfd=%d rfd=%d", p2c[1], c2p[0]);

    int rc = 0;
    for (int i = 0; i < nfiles; i++) {
        if (parent_copy_file(p2c[1], c2p[0], files[i]) < 0)
            rc = 1;   /* ошибка одного файла не останавливает остальные */
    }
    parent_send_end(p2c[1]);

    close(p2c[1]);
    close(c2p[0]);

    int st = 0;
    if (waitpid(pid, &st, 0) < 0) {
        log_msg("main", "waitpid: %s", strerror(errno));
        return 1;
    }
    log_msg("main", "потомок завершился (status=0x%x)", st);
    return rc;
}

/* ---------- именованные каналы ---------- */

static int run_fifo(const char* base, char** files, int nfiles) {
    char fn_p2c[BLOCK_SIZE];
    char fn_c2p[BLOCK_SIZE];

    snprintf(fn_p2c, sizeof(fn_p2c), "%s.p2c", base);
    snprintf(fn_c2p, sizeof(fn_c2p), "%s.c2p", base);

    unlink(fn_p2c);
    unlink(fn_c2p);
    if (mkfifo(fn_p2c, 0600) < 0) die("mkfifo p2c");
    if (mkfifo(fn_c2p, 0600) < 0) die("mkfifo c2p");

    log_msg("main", "созданы именованные каналы: %s %s", fn_p2c, fn_c2p);

    pid_t pid = fork();
    if (pid < 0) die("fork");

    if (pid == 0) {
        /* РЕБЕНОК */
        // 1. Открываем p2c на ЧТЕНИЕ (разблокирует родителя)
        int rfd = open(fn_p2c, O_RDONLY);
        if (rfd < 0) die("потомок: open p2c");

        // 2. Открываем c2p на ЗАПИСЬ (блокируется, ждет родителя)
        int wfd = open(fn_c2p, O_WRONLY);
        if (wfd < 0) die("потомок: open c2p");

        log_msg("child", "запуск (fifo), rfd=%d wfd=%d", rfd, wfd);
        child_run(rfd, wfd);
        _exit(EXIT_FAILURE);
    }

    /* РОДИТЕЛЬ */
    // 1. Открываем p2c на ЗАПИСЬ (блокируется, ждет ребенка)
    int wfd = open(fn_p2c, O_WRONLY);
    if (wfd < 0) die("родитель: open p2c");

    // 2. Открываем c2p на ЧТЕНИЕ (разблокирует ребенка)
    int rfd = open(fn_c2p, O_RDONLY);
    if (rfd < 0) die("родитель: open c2p");

    log_msg("parent", "запуск (fifo), wfd=%d rfd=%d", wfd, rfd);

    int rc = 0;
    for (int i = 0; i < nfiles; i++) {
        if (parent_copy_file(wfd, rfd, files[i]) < 0)
            rc = 1;
    }
    parent_send_end(wfd);

    close(wfd);
    close(rfd);

    int st = 0;
    waitpid(pid, &st, 0);
    log_msg("main", "потомок завершился (status=0x%x)", st);

    unlink(fn_p2c);
    unlink(fn_c2p);
    return rc;
}

/* ---------- main ---------- */

int main(int argc, char** argv) {
    const char* fifo_base = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
        case 'p':
            fifo_base = optarg;
            break;
        default:
            usage(argv[0]);
        }
    }
    if (optind >= argc)
        usage(argv[0]);

    int nfiles = argc - optind;
    char** files = &argv[optind];

    log_msg("main", "режим=%s файлов=%d",
        fifo_base ? "именованный" : "анонимный", nfiles);

    int rc = fifo_base
        ? run_fifo(fifo_base, files, nfiles)
        : run_anon(files, nfiles);

    log_msg("main", "завершено, код возврата=%d", rc);
    return rc;
}