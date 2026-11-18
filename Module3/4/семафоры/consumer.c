#include "config.h"
#include "shm_list.h"

#include <stdio.h>
#include <unistd.h>

/* Найти min/max в блоке по оффсету. Возвращает 0, если блок пуст
 * (не должно случиться: find_unprocessed отдаёт только count > 0). */
static int analyze_block(ShmList* l, size_t off, int* out_n,
    payload_t* out_min, payload_t* out_max)
{
    BlockHdr* h = (BlockHdr*)(shml_base(l) + off);
    int n = h->count;
    if (n <= 0) return 0;

    payload_t* a = cfg_block_body(shml_base(l), off);
    payload_t mn = a[0], mx = a[0];
    for (int i = 1; i < n; i++) {
        if (a[i] < mn) mn = a[i];
        if (a[i] > mx) mx = a[i];
    }
    *out_n = n; *out_min = mn; *out_max = mx;
    return 1;
}

int main(void)
{
    ShmList* l = shml_open(SHM_ROLE_ATTACH);
    if (!l) {
        CFG_LOG("consumer[%d]: shml_open failed\n", getpid());
        return CFG_EXIT_OPEN_FAIL;
    }

    for (;;) {
        int processed = 0;

        shml_lock(l);

        size_t off = shml_find_unprocessed(l);
        if (off != CFG_OFFSET_NONE) {
            int n; payload_t mn, mx;
            if (analyze_block(l, off, &n, &mn, &mx)) {
                /* Вывод — под локом: проще и безопаснее, чем
                 * отпускать лок и печатать «в воздух». */
                printf("consumer[%d]: off=%zu n=%d min=%d max=%d\n",
                    getpid(), off, n, mn, mx);
                fflush(stdout);

                BlockHdr* h = (BlockHdr*)(shml_base(l) + off);
                h->count = 0;          /* помечаем обработанным */
                processed = 1;
            }
        }

        done_flag_t done = shml_get_done(l);
        shml_unlock(l);

        if (processed) {
            usleep(CFG_CONSUMER_WORK_US);
            continue;
        }

        if (done) {
            /* Производитель закончил. Проверим, что всё разобрано */
            int empty;
            shml_lock(l);
            empty = (shml_find_unprocessed(l) == CFG_OFFSET_NONE);
            shml_unlock(l);

            if (empty) {
                printf("consumer[%d]: всё обработано, выхожу\n", getpid());
                break;
            }
        }

        usleep(CFG_CONSUMER_IDLE_US);
    }

    shml_close(l, 0);
    return CFG_EXIT_OK;
}