#include "list.h"
#include "quartile.h"
#include <stdlib.h>
#include <string.h>

int list_cmp_contact(const Contact* a, const Contact* b)
{
    int r;
    if ((r = strcmp(a->Fam, b->Fam)) != 0) return r;
    if ((r = strcmp(a->Im, b->Im)) != 0) return r;
    if ((r = strcmp(a->Otch, b->Otch)) != 0) return r;
    if ((r = strcmp(a->workplace, b->workplace)) != 0) return r;
    if ((r = strcmp(a->position, b->position)) != 0) return r;
    return 0;
}

List* list_create(void)
{
    List* l = (List*)malloc(sizeof(List));
    if (!l) return NULL;
    l->head = l->tail = NULL;
    l->size = 0;
    for (int i = 0; i < 3; ++i) { l->q[i] = NULL; l->score[i] = 0; }
    l->q_active = false;
    return l;
}

void list_destroy(List* list)
{
    if (!list) return;
    Node* cur = list->head;
    while (cur) { Node* nxt = cur->next; free(cur); cur = nxt; }
    free(list);
}

/* вставка: старт от сектора (квартили), привязка, обновление индекса.
   0 — успех, -1 — дубликат, -100 — прочее. */
static int _insert_node_sorted_impl(List* list, Node* n)
{
    if (!list || !n) return -100;

    Node* cur = list->q_active
        ? _qi_find_cmp_start(list, &n->data)
        : list->head;

    while (cur && list_cmp_contact(&cur->data, &n->data) < 0)
        cur = cur->next;

    if (cur && list_cmp_contact(&cur->data, &n->data) == 0)
        return -1;                       /* дубликат */

    n->prev = cur ? cur->prev : list->tail;
    n->next = cur;
    if (n->prev) n->prev->next = n; else list->head = n;
    if (cur)     cur->prev = n;     else list->tail = n;
    list->size++;

    _qi_on_insert(list, n);
    return 0;
}

int list_insert_sorted(List* list, const Contact* c)
{
    if (!list || !c) return -100;

    Node* n = (Node*)malloc(sizeof(Node));
    if (!n) return -100;
    n->data = *c;
    n->prev = n->next = NULL;

    int rc = _insert_node_sorted_impl(list, n);
    if (rc != 0) { free(n); return rc; }   /* -1 дубликат / -100 прочее */
    return 0;
}

int list_insert_node_sorted(List* list, Node* n)
{
    if (!list || !n) return -100;
    n->prev = n->next = NULL;
    return _insert_node_sorted_impl(list, n);
}

static bool _node_in_list(List* list, Node* n)
{
    for (Node* cur = list->head; cur; cur = cur->next)
        if (cur == n) return true;
    return false;
}

bool list_unlink_node(List* list, Node* n)
{
    if (!list || !n) return false;
    if (!_node_in_list(list, n)) return false;

    if (n->prev) n->prev->next = n->next; else list->head = n->next;
    if (n->next) n->next->prev = n->prev; else list->tail = n->prev;
    n->prev = n->next = NULL;

    _qi_on_remove(list, n);   /* score/сдвиги (size ещё старый) */
    list->size--;
    if (list->q_active && list->size < QUARTILE_MIN) _qi_deactivate(list);
    return true;
}

bool list_remove_node(List* list, Node* n)
{
    if (!list_unlink_node(list, n)) return false;
    free(n);
    return true;
}

Node* list_find_cmp(List* list, const Contact* key)
{
    if (!list || !key) return NULL;
    Node* cur = _qi_find_cmp_start(list, key);
    for (; cur; cur = cur->next) {
        int r = list_cmp_contact(&cur->data, key);
        if (r == 0) return cur;
        if (r > 0)  return NULL;
    }
    return NULL;
}

Node* list_find_prefix_start(List* list, const char* prefix)
{
    if (!list || !prefix) return NULL;
    return _qi_find_prefix_start(list, prefix);
}

void list_foreach(List* list, void (*fn)(const Contact*, void*), void* ctx)
{
    if (!list || !fn) return;
    for (Node* cur = list->head; cur; cur = cur->next)
        fn(&cur->data, ctx);
}

size_t list_size(const List* list) { return list ? list->size : 0; }
Node* list_head(const List* list) { return list ? list->head : NULL; }
Node* list_tail(const List* list) { return list ? list->tail : NULL; }