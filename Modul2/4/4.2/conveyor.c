#include "conveyor.h"
#include <string.h>

void cv_init(Conveyor* q) {
    memset(q, 0, sizeof * q);

    q->pool[CV_CAP - 1].next_arr = NULL;
    q->free_list = &q->pool[0];
    for (int i = CV_CAP - 2; i >= 0; --i)
        q->pool[i].next_arr = &q->pool[i + 1];
}

int cv_size(const Conveyor* q) { return q->size; }
int cv_is_empty(const Conveyor* q) { return q->size == 0; }

static void unlink_node(Conveyor* q, ConveyorNode* x) {
    if (x->prev_arr) x->prev_arr->next_arr = x->next_arr;
    else             q->head_arr = x->next_arr;
    if (x->next_arr) x->next_arr->prev_arr = x->prev_arr;
    else             q->tail_arr = x->prev_arr;

    if (x->prev_prio) x->prev_prio->next_prio = x->next_prio;
    else              q->head_prio = x->next_prio;
    if (x->next_prio) x->next_prio->prev_prio = x->prev_prio;
    else              q->tail_prio = x->prev_prio;

    x->next_prio = x->prev_prio = NULL;
    x->next_arr = x->prev_arr = NULL;
    --q->size;
}

int cv_push(Conveyor* q, int id, uint8_t prio, int* evicted_id) {
    if (evicted_id) *evicted_id = -1;

    if (!q->free_list) {
        ConveyorNode* victim = q->head_arr;
        if (!victim) return 0;
        if (evicted_id) *evicted_id = victim->id;
        unlink_node(q, victim);
        victim->next_arr = q->free_list;
        q->free_list = victim;
        ++q->evictions;
    }

    ConveyorNode* n = q->free_list;
    q->free_list = q->free_list->next_arr;

    n->id = id;
    n->prio = prio;
    n->next_prio = n->prev_prio = NULL;
    n->next_arr = n->prev_arr = NULL;

    ConveyorNode* cur = q->head_prio;
    while (cur && cur->prio <= prio)
        cur = cur->next_prio;

    if (cur) {
        n->next_prio = cur;
        n->prev_prio = cur->prev_prio;
        if (cur->prev_prio) cur->prev_prio->next_prio = n;
        else                q->head_prio = n;
        cur->prev_prio = n;
    }
    else {
        n->prev_prio = q->tail_prio;
        if (q->tail_prio) q->tail_prio->next_prio = n;
        else              q->head_prio = n;
        q->tail_prio = n;
    }

    n->prev_arr = q->tail_arr;
    if (q->tail_arr) q->tail_arr->next_arr = n;
    else             q->head_arr = n;
    q->tail_arr = n;

    ++q->size;
    ++q->pushes;
    return 1;
}

ConveyorNode* cv_pop_first(Conveyor* q) {
    ConveyorNode* x = q->head_prio;
    if (!x) return NULL;
    unlink_node(q, x);
    ++q->pops;
    return x;
}

ConveyorNode* cv_pop_min_priority(Conveyor* q, uint8_t p) {
    for (ConveyorNode* cur = q->head_prio; cur; cur = cur->next_prio) {
        if (cur->prio >= p) {
            unlink_node(q, cur);
            ++q->pops;
            return cur;
        }
    }
    return NULL;
}

void cv_release(Conveyor* q, ConveyorNode* n) {
    n->next_arr = q->free_list;
    q->free_list = n;
}

const ConveyorNode* cv_first_arr(const Conveyor* q) { return q->head_arr; }
const ConveyorNode* cv_next_arr(const ConveyorNode* n) { return n->next_arr; }

const ConveyorNode* cv_first_prio(const Conveyor* q) { return q->head_prio; }
const ConveyorNode* cv_next_prio(const ConveyorNode* n) { return n->next_prio; }