/* ============================================================
 * cleaner.c — ручной уборщик очередей.
 * ============================================================ */
#define _POSIX_C_SOURCE 200809L

#include "cleaner.h"
#include "common.h"
#include "config.h"      /* NAME_MAX_LEN */

#include <errno.h>
#include <mqueue.h>
#include <stdio.h>

int clean_queues(const char* base) {
    char name1[NAME_MAX_LEN], name2[NAME_MAX_LEN];
    build_names(base, name1, name2, sizeof name1);
    int rc = 0;

    if (mq_unlink(name1) == 0)      printf("removed %s\n", name1);
    else if (errno != ENOENT) { perror("mq_unlink(_1)"); rc = -1; }

    if (mq_unlink(name2) == 0)      printf("removed %s\n", name2);
    else if (errno != ENOENT) { perror("mq_unlink(_2)"); rc = -1; }

    if (rc == 0) printf("clean done\n");
    return rc;
}