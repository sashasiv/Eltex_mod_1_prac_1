#include "list.h"
#include <stdio.h>
#include <string.h>

static Contact make(const char* fam, const char* im)
{
    Contact c; memset(&c, 0, sizeof(c));
    strncpy(c.Fam, fam, STR_LEN - 1);
    strncpy(c.Im, im, STR_LEN - 1);
    return c;
}

static void print_contact(const Contact* c, void* ctx)
{
    (void)ctx;
    printf(" %s %s\n", c->Fam, c->Im);
}

static void check(int cond, const char* msg)
{
    if (!cond) printf("FAIL: %s\n", msg);
}

/* позиция узла в списке (0-based); -1 если не найден */
static int node_pos(List* l, Node* target)
{
    int i = 0;
    for (Node* cur = l->head; cur; cur = cur->next, ++i)
        if (cur == target) return i;
    return -1;
}

static void print_quartiles(List* l)
{
    printf("  q_active=%d size=%zu\n", (int)l->q_active, l->size);
    if (!l->q_active) { printf("  (quartiles inactive)\n"); return; }
    for (int i = 0; i < 3; ++i) {
        if (l->q[i]) {
            printf("  q%d: pos=%d score=%d  Fam=%s Im=%s\n",
                   i + 1, node_pos(l, l->q[i]), l->score[i],
                   l->q[i]->data.Fam, l->q[i]->data.Im);
        }
        else {
            printf("  q%d: NULL\n", i + 1);
        }
    }
}

int main(void)
{
    List* l = list_create();
    if (!l) { printf("create failed\n"); return 1; }

    /* --- базовые проверки --- */
    Contact a = make("Ivanov", "Ivan");
    Contact b = make("Petrov", "Petr");
    Contact c = make("Sidorov", "Sidr");
    Contact d = make("Ivanov", "Ivan");   /* дубликат a */

    check(list_insert_sorted(l, &b) == 0, "insert b");
    check(list_insert_sorted(l, &a) == 0, "insert a");
    check(list_insert_sorted(l, &c) == 0, "insert c");
    check(list_insert_sorted(l, &d) == -1, "duplicate d rejected");   /* -1 = дубликат */

    printf("after 3 inserts (size=%zu, q_active=%d):\n", list_size(l), (int)l->q_active);
    list_foreach(l, print_contact, NULL);

    /* --- доводим до 7 (активация при 7) --- */
    Contact e = make("Alekseev", "Al");
    Contact f = make("Borisov", "Bo");
    Contact g = make("Fedorov", "Fe");
    Contact h = make("Grigoriev", "Gr");
    list_insert_sorted(l, &e);
    list_insert_sorted(l, &f);
    list_insert_sorted(l, &g);
    list_insert_sorted(l, &h);

    printf("\nafter 7 inserts (size=%zu):\n", list_size(l));
    list_foreach(l, print_contact, NULL);
    print_quartiles(l);
    check(list_size(l) == 7, "size == 7");
    check(l->q_active, "q_active after n=7");
    check(l->q[0] && l->q[1] && l->q[2], "q[0..2] set at n=7");

    /* --- точный поиск через сектор --- */
    Contact key = make("Petrov", "Petr");
    Node* n = list_find_cmp(l, &key);
    check(n != NULL, "find Petrov via quartile");

    /* --- префиксный старт --- */
    Node* ps = list_find_prefix_start(l, "Iv");
    check(ps != NULL, "prefix start 'Iv'");
    printf("prefix start 'Iv': %s %s\n", ps ? ps->data.Fam : "-", ps ? ps->data.Im : "-");

    /* --- добавляем до 19 --- */
    const char* fams[12] = {
        "Zaitsev","Kuznetsov","Morozov","Novikov","Orlov",
        "Pavlov","Romanov","Sokolov","Tarasov","Ustinov",
        "Volkov","Yakovlev"
    };
    const char* ims[12] = {
        "Za","Ku","Mo","No","Or","Pa","Ro","So","Ta","Us","Vo","Ya"
    };
    for (int i = 0; i < 12; ++i) {
        Contact x = make(fams[i], ims[i]);
        check(list_insert_sorted(l, &x) == 0, fams[i]);
    }

    printf("\nafter 19 inserts (size=%zu):\n", list_size(l));
    list_foreach(l, print_contact, NULL);
    print_quartiles(l);
    check(list_size(l) == 19, "size == 19");
    check(l->q_active, "q_active at n=19");

    /* --- поиск в разных секторах --- */
    Contact k1 = make("Alekseev", "Al");
    Contact k2 = make("Ivanov", "Ivan");
    Contact k3 = make("Zaitsev", "Za");
    check(list_find_cmp(l, &k1) != NULL, "find Alekseev (sector1)");
    check(list_find_cmp(l, &k2) != NULL, "find Ivanov (middle)");
    check(list_find_cmp(l, &k3) != NULL, "find Zaitsev (sector4)");

    /* --- re-sort Petrov -> Zaharov --- */
    if (n) {
        check(list_unlink_node(l, n), "unlink Petrov");
        strncpy(n->data.Fam, "Zaharov", STR_LEN - 1);
        check(list_insert_node_sorted(l, n) == 0, "reinsert Zaharov");
    }
    printf("\nafter re-sort (size=%zu):\n", list_size(l));
    print_quartiles(l);

    /* --- удаление до n < 7: деактивация --- */
    int removed = 0;
    while (list_size(l) > 6) {
        Node* victim = list_head(l);
        if (!victim) break;
        list_remove_node(l, victim);
        removed++;
    }
    printf("\nafter removing %d (size=%zu, q_active=%d):\n",
           removed, list_size(l), (int)l->q_active);
    print_quartiles(l);
    check(list_size(l) == 6, "size == 6");
    check(!l->q_active, "q_active false after n<7");

    list_destroy(l);
    printf("done\n");
    return 0;
}