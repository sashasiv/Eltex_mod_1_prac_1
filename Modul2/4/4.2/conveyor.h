#ifndef CONVEYOR_H
#define CONVEYOR_H

#include <stdint.h>

#define CV_CAP       16
#define CV_PRIO_MAX  256

typedef enum { POP_FIRST = 0, POP_MIN_PRIO = 1 } PopPolicy;

typedef struct ConveyorNode {
    int      id;
    uint8_t  prio;
    struct ConveyorNode* next_prio, * prev_prio;
    struct ConveyorNode* next_arr, * prev_arr;
} ConveyorNode;

typedef struct {
    ConveyorNode pool[CV_CAP];
    ConveyorNode* free_list;

    ConveyorNode* head_prio, * tail_prio;
    ConveyorNode* head_arr, * tail_arr;

    int size;
    unsigned long pushes, pops, evictions;
} Conveyor;

void cv_init(Conveyor* q);
int  cv_size(const Conveyor* q);
int  cv_is_empty(const Conveyor* q);

int cv_push(Conveyor* q, int id, uint8_t prio, int* evicted_id);

ConveyorNode* cv_pop_first(Conveyor* q);
ConveyorNode* cv_pop_min_priority(Conveyor* q, uint8_t p);

void cv_release(Conveyor* q, ConveyorNode* n);

const ConveyorNode* cv_first_arr(const Conveyor* q);
const ConveyorNode* cv_next_arr(const ConveyorNode* n);

/* conveyor.h */
const ConveyorNode* cv_first_prio(const Conveyor* q);
const ConveyorNode* cv_next_prio(const ConveyorNode* n);

#endif