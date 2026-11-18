#include "shm_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

/* Специфика POSIX. */
typedef struct {
    sem_t* sem;
} PxImpl;

ShmList* shml_open(ShmRole role)
{
    ShmList* l = calloc(1, sizeof(*l));
    PxImpl* im = calloc(1, sizeof(*im));
    if (!l || !im) { free(l); free(im); return NULL; }

    l->size = CFG_SHM_SIZE;
    l->impl = im;

    int flags = (role == SHM_ROLE_CREATE) ? (O_CREAT | O_RDWR) : O_RDWR;
    int fd = shm_open(CFG_PX_SHM_NAME, flags, 0666);
    if (fd < 0) { perror("shm_open"); goto fail; }

    /* ftruncate обязателен при создании; при attach размер уже задан,
     * но повторный вызов с тем же размером безвреден. */
    if (role == SHM_ROLE_CREATE && ftruncate(fd, CFG_SHM_SIZE) < 0) {
        perror("ftruncate"); close(fd); goto fail;
    }

    l->base = mmap(NULL, CFG_SHM_SIZE, PROT_READ | PROT_WRITE,
        MAP_SHARED, fd, 0);
    close(fd);
    if (l->base == MAP_FAILED) { perror("mmap"); l->base = NULL; goto fail; }

    im->sem = (role == SHM_ROLE_CREATE)
        ? sem_open(CFG_PX_SEM_NAME, O_CREAT, 0666, 1)
        : sem_open(CFG_PX_SEM_NAME, 0);
    if (im->sem == SEM_FAILED) {
        perror("sem_open");
        munmap(l->base, CFG_SHM_SIZE);
        l->base = NULL;
        goto fail;
    }

    if (role == SHM_ROLE_CREATE)
        memset(l->base, 0, CFG_SHM_SIZE);   /* пустой список, done = 0 */

    return l;

fail:
    free(im);
    free(l);
    return NULL;
}

void shml_close(ShmList* l, int unlink_objects)
{
    if (!l) return;
    PxImpl* im = l->impl;

    if (l->base && l->base != MAP_FAILED)
        munmap(l->base, l->size);

    if (im && im->sem && im->sem != SEM_FAILED)
        sem_close(im->sem);

    if (unlink_objects) {
        shm_unlink(CFG_PX_SHM_NAME);
        sem_unlink(CFG_PX_SEM_NAME);
    }

    free(im);
    free(l);
}

void shml_lock(ShmList* l)
{
    if (sem_wait(((PxImpl*)l->impl)->sem) < 0)
        perror("sem_wait");
}

void shml_unlock(ShmList* l)
{
    if (sem_post(((PxImpl*)l->impl)->sem) < 0)
        perror("sem_post");
}