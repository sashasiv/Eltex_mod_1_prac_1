#include "shm_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

/* Специфика System V. */
typedef struct {
    int shmid;
    int semid;
} SvImpl;

/* --- Внутренние обёртки над семафором ------------------------------------- */

static void sv_lock(SvImpl* im)
{
    struct sembuf op = { .sem_num = 0, .sem_op = -1, .sem_flg = 0 };
    if (semop(im->semid, &op, 1) < 0)
        perror("semop lock");
}

static void sv_unlock(SvImpl* im)
{
    struct sembuf op = { .sem_num = 0, .sem_op = +1, .sem_flg = 0 };
    if (semop(im->semid, &op, 1) < 0)
        perror("semop unlock");
}

/* --- shml_open ------------------------------------------------------------ */

ShmList* shml_open(ShmRole role)
{
    ShmList* l = calloc(1, sizeof(*l));
    SvImpl* im = calloc(1, sizeof(*im));
    if (!l || !im) { free(l); free(im); return NULL; }

    l->size = CFG_SHM_SIZE;
    l->impl = im;

    if (role == SHM_ROLE_CREATE) {
        im->shmid = shmget(CFG_SV_SHM_KEY, CFG_SHM_SIZE, IPC_CREAT | 0666);
        if (im->shmid < 0) { perror("shmget create"); goto fail; }

        im->semid = semget(CFG_SV_SEM_KEY, 1, IPC_CREAT | 0666);
        if (im->semid < 0) { perror("semget create"); goto fail; }

        if (semctl(im->semid, 0, SETVAL, 1) < 0) {
            perror("semctl SETVAL"); goto fail;
        }

        l->base = shmat(im->shmid, NULL, 0);
        if (l->base == (char*)-1) { perror("shmat create"); l->base = NULL; goto fail; }

        memset(l->base, 0, CFG_SHM_SIZE);   /* пустой список, done = 0 */
    }
    else {
        im->shmid = shmget(CFG_SV_SHM_KEY, 0, 0666);
        if (im->shmid < 0) { perror("shmget attach"); goto fail; }

        im->semid = semget(CFG_SV_SEM_KEY, 0, 0666);
        if (im->semid < 0) { perror("semget attach"); goto fail; }

        l->base = shmat(im->shmid, NULL, 0);
        if (l->base == (char*)-1) { perror("shmat attach"); l->base = NULL; goto fail; }
    }
    return l;

fail:
    free(im);
    free(l);
    return NULL;
}

/* --- shml_close ----------------------------------------------------------- */

void shml_close(ShmList* l, int unlink_objects)
{
    if (!l) return;
    SvImpl* im = l->impl;

    if (l->base && l->base != (char*)-1)
        shmdt(l->base);

    if (im) {
        if (unlink_objects) {
            shmctl(im->shmid, IPC_RMID, NULL);
            semctl(im->semid, 0, IPC_RMID);
        }
        free(im);
    }
    free(l);
}

/* --- Синхронизация -------------------------------------------------------- */

void shml_lock(ShmList* l) { sv_lock((SvImpl*)l->impl); }
void shml_unlock(ShmList* l) { sv_unlock((SvImpl*)l->impl); }