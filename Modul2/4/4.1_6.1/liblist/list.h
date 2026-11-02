#ifndef LIST_H
#define LIST_H

#define _CRT_SECURE_NO_WARNINGS
#include <stdbool.h>
#include <stddef.h>
#include "contact.h"

#define QUARTILE_MIN 7

typedef struct Node {
    Contact      data;
    struct Node* prev;
    struct Node* next;
} Node;

typedef struct List {
    Node* head;
    Node* tail;
    size_t size;

    Node* q[3];
    int    score[3];
    bool   q_active;
} List;

/* публичный компаратор (нужен edit_contact в phonebook.c) */
int list_cmp_contact(const Contact* a, const Contact* b);

List* list_create(void);
void  list_destroy(List* list);

/* вставка нового узла (malloc + копия):
   0 — успех, -1 — дубликат, -100 — прочее (malloc, NULL). */
int list_insert_sorted(List* list, const Contact* c);

/* вставка существующего узла (для re-sort; без malloc):
   0 — успех, -1 — дубликат, -100 — прочее (NULL). */
int list_insert_node_sorted(List* list, Node* n);

bool list_unlink_node(List* list, Node* n);
bool list_remove_node(List* list, Node* n);

Node* list_find_cmp(List* list, const Contact* key);
Node* list_find_prefix_start(List* list, const char* prefix);

void   list_foreach(List* list, void (*fn)(const Contact*, void*), void* ctx);
size_t list_size(const List* list);
Node* list_head(const List* list);
Node* list_tail(const List* list);

#endif