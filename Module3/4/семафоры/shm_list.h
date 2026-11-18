#ifndef SHM_LIST_H
#define SHM_LIST_H

#include <stddef.h>
#include "config.h"

/*
 * Слой хранения: односвязный список блоков в разделяемой памяти.
 * Не знает, какая IPC используется (System V / POSIX) — это решает
 * бэкенд на этапе линковки.
 *
 * Наружу отдаются ОФФСЕТЫ, а не указатели: сегмент может быть
 * отображён по разным адресам в разных процессах.
 * Все операции над списком — под shml_lock()/shml_unlock().
 */

 /* Поля base/size — общие для всех бэкендов и нужны слою ops.
  * Поле impl — непрозрачно: специфика бэкенда; пользователь API
  * не должен его трогать. */
struct ShmList {
    char* base;
    size_t  size;
    void* impl;
};
typedef struct ShmList ShmList;

typedef enum {
    SHM_ROLE_CREATE,
    SHM_ROLE_ATTACH
} ShmRole;

ShmList* shml_open(ShmRole role);
void     shml_close(ShmList* l, int unlink_objects);

char* shml_base(ShmList* l);
size_t   shml_size(ShmList* l);

void shml_lock(ShmList* l);
void shml_unlock(ShmList* l);

done_flag_t shml_get_done(ShmList* l);
void        shml_set_done(ShmList* l);

size_t shml_find_unprocessed(ShmList* l);
size_t shml_append_block(ShmList* l, size_t at,
    size_t* prev_last_off, int count);

#endif /* SHM_LIST_H */