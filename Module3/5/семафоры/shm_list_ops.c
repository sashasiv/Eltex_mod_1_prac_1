#include "shm_list.h"

/*
 * Всё, что не зависит от выбранной IPC.
 */

//вывод во вне
char* shml_base(ShmList* l) { return l->base; }
size_t  shml_size(ShmList* l) { return l->size; }

// работа с флагом shml_get_done (проверить/поставить)
done_flag_t shml_get_done(ShmList* l)
{
    return *(done_flag_t*)(l->base + CFG_DONE_OFFSET);
}

void shml_set_done(ShmList* l)
{
    *(done_flag_t*)(l->base + CFG_DONE_OFFSET) = 1;
}
// поиск непройденного блока
size_t shml_find_unprocessed(ShmList* l)
{
    size_t off = cfg_first_block_offset();
    while (off != CFG_OFFSET_NONE) {
        BlockHdr* h = (BlockHdr*)(l->base + off);
        if (h->count > 0)
            return off;
        off = (size_t)h->next;
    }
    return CFG_OFFSET_NONE;
}
// добавление блока
size_t shml_append_block(ShmList* l, size_t at,
    size_t* prev_last_off, int count)
{
    size_t need = cfg_block_size(count);
    if (at + need > l->size)
        return CFG_OFFSET_NONE;

    BlockHdr* h = (BlockHdr*)(l->base + at);
    h->next = 0;
    /* count — последним: до этого момента потребитель видит count == 0
     * и блок «не существует» для него. */
    if (*prev_last_off != CFG_OFFSET_NONE) {
        BlockHdr* p = (BlockHdr*)(l->base + *prev_last_off);
        p->next = (int)at;
    }
    h->count = count;

    *prev_last_off = at;
    return at;
}