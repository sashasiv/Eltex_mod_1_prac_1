#include "config.h"
#include "shm_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/* Сгенерировать n чисел в body. */
static void fill_body(payload_t* body, int n)
{
    for (int i = 0; i < n; i++) {
        body[i] = CFG_VALUE_MIN +
            rand() % (CFG_VALUE_MAX - CFG_VALUE_MIN + 1);
    }
}

/* Случайное n из [CFG_BLOCK_MIN_ELEMS, CFG_BLOCK_MAX_ELEMS]. */
static int random_block_len(void)
{
    return CFG_BLOCK_MIN_ELEMS +
        rand() % (CFG_BLOCK_MAX_ELEMS - CFG_BLOCK_MIN_ELEMS + 1);
}

int main(void)
{
    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    ShmList* l = shml_open(SHM_ROLE_CREATE);
    if (!l) {
        CFG_LOG("producer: shml_open failed\n");
        return CFG_EXIT_OPEN_FAIL;
    }

    size_t offset = cfg_first_block_offset();  /* куда класть следующий блок */
    size_t last = CFG_OFFSET_NONE;            /* оффсет текущего хвоста */
    int    blocks = 0;

    for (;;) {
        int n = random_block_len();
        size_t need = cfg_block_size(n);

        shml_lock(l);

        /* Проверка места — до записи тела. */
        if (offset + need > shml_size(l)) {
            shml_unlock(l);
            CFG_LOG("producer: память исчерпана после %d блоков\n", blocks);
            break;
        }

        /* Тело заполняем ДО append: блок станет виден потребителям
         * уже готовым (count выставляется последним внутри append). */
        payload_t* body = cfg_block_body(shml_base(l), offset);
        fill_body(body, n);

        if (shml_append_block(l, offset, &last, n) == CFG_OFFSET_NONE) {
            /* Сюда попасть не должны: место проверено выше.
             * Но если append отказал — не зацикливаемся. */
            shml_unlock(l);
            CFG_LOG("producer: append failed\n");
            break;
        }
        offset += need;
        blocks++;

        shml_unlock(l);
        usleep(CFG_PRODUCER_SLEEP_US);
    }

    /* Сообщаем потребителям: новых блоков не будет. */
    shml_lock(l);
    shml_set_done(l);
    shml_unlock(l);

    printf("producer: завершено, блоков=%d\n", blocks);
    shml_close(l, 0);          /* ресурсы НЕ удаляем — их судьба у run/потребителей */
    return CFG_EXIT_OK;
}