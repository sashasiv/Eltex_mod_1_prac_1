#include "phonebook.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---------- глобальное состояние ---------- */

static List* g_list = NULL;

/* ---------- внутренние ---------- */

static void _copy_str(char* dst, const char* src)
{
    if (!src) { dst[0] = '\0'; return; }
    strncpy(dst, src, STR_LEN - 1);
    dst[STR_LEN - 1] = '\0';
}

static bool _is_blank(const char* s)
{
    if (!s) return true;
    for (int i = 0; s[i]; ++i) {
        unsigned char ch = (unsigned char)s[i];
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r' &&
            ch != '\v' && ch != '\f')
            return false;
    }
    return true;
}

/* массивы строк внутри Contact (не путать с g_list) */
static bool _add_to_arr(char arr[][STR_LEN], int* count, const char* value)
{
    if (_is_blank(value)) return false;
    if (*count >= ARR_LEN) return false;
    _copy_str(arr[*count], value);
    (*count)++;
    return true;
}

static int _remove_from_arr(char arr[][STR_LEN], int* count, int index)
{
    if (index < 0 || index >= *count) return 0;
    for (int j = index; j < *count - 1; ++j)
        _copy_str(arr[j], arr[j + 1]);
    (*count)--;
    return 1;
}

static int _parse_index(const char* value)
{
    if (!value || value[0] < '1' || value[0] > '9') return 0;
    return value[0] - '0';
}

static bool _set_field(Contact* c, const char* field, const char* value)
{
    if (strcmp(field, "Fam") == 0) {
        if (_is_blank(value)) return false;
        _copy_str(c->Fam, value);
    }
    else if (strcmp(field, "Im") == 0) {
        if (_is_blank(value)) return false;
        _copy_str(c->Im, value);
    }
    else if (strcmp(field, "Otch") == 0) {
        if (_is_blank(value)) c->Otch[0] = '\0';
        else                  _copy_str(c->Otch, value);
    }
    else if (strcmp(field, "workplace") == 0) {
        if (_is_blank(value)) c->workplace[0] = '\0';
        else                  _copy_str(c->workplace, value);
    }
    else if (strcmp(field, "position") == 0) {
        if (_is_blank(value)) c->position[0] = '\0';
        else                  _copy_str(c->position, value);
    }
    else if (strcmp(field, "phone_add") == 0) {
        if (!_add_to_arr(c->phones, &c->phones_count, value)) return false;
    }
    else if (strcmp(field, "email_add") == 0) {
        if (!_add_to_arr(c->emails, &c->emails_count, value)) return false;
    }
    else if (strcmp(field, "social_add") == 0) {
        if (!_add_to_arr(c->socials, &c->socials_count, value)) return false;
    }
    else if (strcmp(field, "messenger_add") == 0) {
        if (!_add_to_arr(c->messengers, &c->messengers_count, value)) return false;
    }
    else if (strcmp(field, "phone_remove") == 0) {
        if (!_remove_from_arr(c->phones, &c->phones_count, _parse_index(value) - 1)) return false;
    }
    else if (strcmp(field, "email_remove") == 0) {
        if (!_remove_from_arr(c->emails, &c->emails_count, _parse_index(value) - 1)) return false;
    }
    else if (strcmp(field, "social_remove") == 0) {
        if (!_remove_from_arr(c->socials, &c->socials_count, _parse_index(value) - 1)) return false;
    }
    else if (strcmp(field, "messenger_remove") == 0) {
        if (!_remove_from_arr(c->messengers, &c->messengers_count, _parse_index(value) - 1)) return false;
    }
    else return false;
    return true;
}

/* ---------- инициализация (8 предустановок) ---------- */

static const Contact PRESET[8] = {
    { .Fam = "Alekseev",  .Im = "Al"   },
    { .Fam = "Borisov",   .Im = "Bo"   },
    { .Fam = "Fedorov",   .Im = "Fe"   },
    { .Fam = "Grigoriev", .Im = "Gr"   },
    { .Fam = "Ivanov",    .Im = "Ivan" },
    { .Fam = "Kuznetsov", .Im = "Ku"   },
    { .Fam = "Petrov",    .Im = "Petr" },
    { .Fam = "Sidorov",   .Im = "Sidr" },
};

void phonebook_init(void)
{
    if (g_list) return;
    g_list = list_create();
    if (!g_list) return;
    for (int i = 0; i < 8; ++i)
        list_insert_sorted(g_list, &PRESET[i]);
}

/* ---------- размер / обход ---------- */

size_t contacts_count(void) { return g_list ? list_size(g_list) : 0; }

void contacts_foreach(void (*fn)(const Contact*, void*), void* ctx)
{
    if (g_list) list_foreach(g_list, fn, ctx);
}

/* ---------- добавление ---------- */

int add_contact(const char* fam, const char* im, ...)
{
    if (!g_list) return -100;
    if (_is_blank(fam) || _is_blank(im)) return -2;   /* пусто */

    Contact c; memset(&c, 0, sizeof(c));
    _copy_str(c.Fam, fam);
    _copy_str(c.Im, im);

    va_list ap; va_start(ap, im);
    const char* key;
    while ((key = va_arg(ap, const char*)) != NULL) {
        const char* val = va_arg(ap, const char*);
        if (!val) break;
        _set_field(&c, key, val);
    }
    va_end(ap);

    return list_insert_sorted(g_list, &c);
}

/* ---------- поиск узла / контакта ---------- */

Node* find_node_by_key(const Contact* key)
{
    if (!g_list || !key) return NULL;
    return list_find_cmp(g_list, key);
}

Contact* find_by_key(const Contact* key)
{
    Node* n = find_node_by_key(key);
    return n ? &n->data : NULL;
}

/* ---------- правка по массиву пар ---------- */
int edit_contact_fields(Node* target, Node** out,
                        const char** keys, const char** vals, int n)
{
    if (out) *out = NULL;
    if (!g_list || !target || !keys || !vals || n <= 0) return -100;

    Contact old = target->data;
    Contact tmp = old;

    for (int i = 0; i < n; ++i) {
        if (!keys[i] || !vals[i]) return -100;
        if (!_set_field(&tmp, keys[i], vals[i])) return -100;
    }

    if (_is_blank(tmp.Fam) || _is_blank(tmp.Im)) return -2;   /* пусто */

    bool key_changed = (list_cmp_contact(&old, &tmp) != 0);

    if (key_changed) {
        Node* dup = list_find_cmp(g_list, &tmp);
        if (dup && dup != target) return -1;                  /* дубликат */
    }

    if (!key_changed) {
        target->data = tmp;
        if (out) *out = target;
        return 0;
    }

    /* re-sort перепривязкой */
    if (!list_unlink_node(g_list, target)) return -100;
    target->data = tmp;
    int rc = list_insert_node_sorted(g_list, target);
    if (rc != 0) {
        /* аномалия: откат (target уже отвязан; вернуть на место) */
        target->data = old;
        list_insert_node_sorted(g_list, target);
        return rc;   /* -1 (не должно) / -100 */
    }
    if (out) *out = target;
    return 0;
}

/* ---------- правка (varargs) — обёртка ---------- */
int edit_contact(Node* target, Node** out, ...)
{
    const char* keys[16];
    const char* vals[16];
    int n = 0;

    va_list ap; va_start(ap, out);
    const char* key;
    while ((key = va_arg(ap, const char*)) != NULL) {
        const char* val = va_arg(ap, const char*);
        if (!val || n >= 16) { va_end(ap); return -100; }
        keys[n] = key;
        vals[n] = val;
        n++;
    }
    va_end(ap);

    return edit_contact_fields(target, out, keys, vals, n);
}
/* ---------- удаление ---------- */

bool remove_contact(Node* target)
{
    if (!g_list || !target) return false;
    return list_remove_node(g_list, target);
}

/* ---------- снимок по префиксу фамилии ---------- */

struct Snapshot {
    Node** items;
    size_t count;
};

Snapshot* snapshot_find_by_fam_prefix(const char* prefix)
{
    if (!g_list || !prefix || !*prefix) return NULL;

    size_t plen = strlen(prefix);
    size_t cap  = list_size(g_list);
    if (cap == 0) return NULL;

    Node** items = (Node**)malloc(cap * sizeof(Node*));
    if (!items) return NULL;

    size_t n = 0;
    Node* cur = list_find_prefix_start(g_list, prefix);
    for (; cur; cur = cur->next) {
        int r = strncmp(cur->data.Fam, prefix, plen);
        if (r > 0) break;
        if (r == 0) items[n++] = cur;
    }

    if (n == 0) { free(items); return NULL; }

    Snapshot* s = (Snapshot*)malloc(sizeof(Snapshot));
    if (!s) { free(items); return NULL; }
    s->items = items;
    s->count = n;
    return s;
}

size_t snapshot_count(const Snapshot* s) { return s ? s->count : 0; }

Node* snapshot_get(const Snapshot* s, int k)
{
    if (!s || k < 1 || (size_t)k > s->count) return NULL;
    return s->items[k - 1];
}

void snapshot_destroy(Snapshot* s)
{
    if (!s) return;
    free(s->items);
    free(s);
}