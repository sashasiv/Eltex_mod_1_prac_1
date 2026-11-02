#include "quartile.h"
#include <string.h>

const QConfig QCFG[3] = {
    { -3, +1, 4 },   /* q1: слева -3, справа +1, порог 4 */
    { -1, +1, 2 },   /* q2: слева -1, справа +1, порог 2 */
    { -1, +3, 4 },   /* q3: слева -1, справа +3, порог 4 */
};

/* ---------- активация / деактивация ---------- */

void _qi_deactivate(List* list)
{
    for (int i = 0; i < 3; ++i) {
        list->q[i] = NULL;
        list->score[i] = 0;
    }
    list->q_active = false;
}

void _qi_activate(List* list)
{
    size_t n = list->size;
    if (n < QUARTILE_MIN) { _qi_deactivate(list); return; }

    size_t pos[3] = { n / 4, n / 2, (3 * n) / 4 };

    Node* cur = list->head;
    size_t idx = 0;
    int    qi = 0;
    while (cur && qi < 3) {
        if (idx == pos[qi]) {
            list->q[qi] = cur;
            list->score[qi] = 0;
            qi++;
        }
        cur = cur->next;
        idx++;
    }
    while (qi < 3) { list->q[qi] = NULL; list->score[qi] = 0; qi++; }
    list->q_active = true;
}

/* ---------- применение баллов ---------- */

static void _qi_apply(List* list, int i)
{
    int thr = QCFG[i].thr;
    int s = list->score[i];

    if (s >= thr) {
        if (list->q[i] && list->q[i]->next) {
            list->q[i] = list->q[i]->next;
            list->score[i] -= thr;
        }
    }
    else if (s <= -thr) {
        if (list->q[i] && list->q[i]->prev) {
            list->q[i] = list->q[i]->prev;
            list->score[i] += thr;
        }
    }
}

/* ---------- обновление при вставке / удалении ---------- */

static void _qi_score_update(List* list, Node* n, bool insert)
{
    for (int i = 0; i < 3; ++i) {
        Node* q = list->q[i];
        if (!q) continue;
        int r = list_cmp_contact(&n->data, &q->data);
        int w = (r < 0) ? QCFG[i].wl : QCFG[i].wr;
        if (!insert) w = -w;
        list->score[i] += w;
    }
}

void _qi_on_insert(List* list, Node* n)
{
    if (!list->q_active) {
        if (list->size >= QUARTILE_MIN) _qi_activate(list);
        return;
    }
    _qi_score_update(list, n, true);
    for (int i = 0; i < 3; ++i) _qi_apply(list, i);
}

void _qi_on_remove(List* list, Node* n)
{
    if (!list->q_active) return;
    _qi_score_update(list, n, false);
    for (int i = 0; i < 3; ++i) _qi_apply(list, i);
}

/* ---------- старт поиска ---------- */

Node* _qi_find_cmp_start(List* list, const Contact* key)
{
    if (!list->q_active) return list->head;

    if (list->q[0] && list_cmp_contact(key, &list->q[0]->data) < 0) return list->head;
    if (list->q[1] && list_cmp_contact(key, &list->q[1]->data) < 0) return list->q[0];
    if (list->q[2] && list_cmp_contact(key, &list->q[2]->data) < 0) return list->q[1];
    return list->q[2] ? list->q[2] : list->head;
}

static int _cmp_fam_prefix(const Contact* c, const char* prefix, size_t plen)
{
    return strncmp(c->Fam, prefix, plen);
}

Node* _qi_find_prefix_start(List* list, const char* prefix)
{
    if (!list->q_active) return list->head;
    size_t plen = strlen(prefix);

    Node* start = list->head;
    if (list->q[0] && _cmp_fam_prefix(&list->q[0]->data, prefix, plen) < 0) start = list->q[0];
    if (list->q[1] && _cmp_fam_prefix(&list->q[1]->data, prefix, plen) < 0) start = list->q[1];
    if (list->q[2] && _cmp_fam_prefix(&list->q[2]->data, prefix, plen) < 0) start = list->q[2];
    return start;
}