#include "phonebook.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static Tree* g_tree = NULL;

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

/* массивы строк внутри Contact */
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
    { .Fam = "Alekseev",  .Im = "Al" },
    { .Fam = "Borisov",   .Im = "Bo" },
    { .Fam = "Fedorov",   .Im = "Fe" },
    { .Fam = "Grigoriev", .Im = "Gr" },
    { .Fam = "Ivanov",    .Im = "Ivan" },
    { .Fam = "Kuznetsov", .Im = "Ku" },
    { .Fam = "Petrov",    .Im = "Petr" },
    { .Fam = "Sidorov",   .Im = "Sidr" },
};

void phonebook_init(void)
{
    if (g_tree) return;
    g_tree = tree_create();
    if (!g_tree) return;
    for (int i = 0; i < 8; ++i)
        tree_insert(g_tree, &PRESET[i], NULL);
}

/* ---------- размер / обход / вывод ---------- */

size_t contacts_count(void) { return g_tree ? g_tree->size : 0; }

void contacts_foreach(Order order, void (*fn)(const Contact*, void*), void* ctx)
{
    if (g_tree) tree_foreach(g_tree->root, order, fn, ctx);
}

void contacts_print_tree(void)
{
    if (!g_tree || !g_tree->root) { printf("(дерево пусто)\n"); return; }
    tree_print(g_tree->root, "", true, true);
}

/* ---------- добавление ---------- */

int add_contact(const char* fam, const char* im, ...)
{
    if (!g_tree) return -100;
    if (_is_blank(fam) || _is_blank(im)) return -2;

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

    return tree_insert(g_tree, &c, NULL);   /* 0 / -1 / -100 — напрямую */
}

/* ---------- поиск ---------- */

TreeNode* find_node_by_key(const Contact* key)
{
    if (!g_tree || !key) return NULL;
    return tree_find(g_tree, key);
}

Contact* find_by_key(const Contact* key)
{
    TreeNode* n = find_node_by_key(key);
    return n ? &n->data : NULL;
}

/* ---------- правка ---------- */

int edit_contact(TreeNode* target, TreeNode** out, ...)
{
    const char* keys[16];
    const char* vals[16];
    int n = 0;
    va_list ap; va_start(ap, out);
    const char* key;
    while ((key = va_arg(ap, const char*)) != NULL) {
        const char* val = va_arg(ap, const char*);
        if (!val || n >= 16) { va_end(ap); return -100; }
        keys[n] = key; vals[n] = val; n++;
    }
    va_end(ap);
    return edit_contact_n(target, out, keys, vals, n);
}

int edit_contact_n(TreeNode* target, TreeNode** out,
    const char** keys, const char** vals, int n)
{
    if (out) *out = NULL;
    if (!g_tree || !target) return -100;
    if (n < 0) return -100;

    Contact tmp = target->data;
    for (int i = 0; i < n; ++i) {
        if (!keys[i] || !vals[i]) return -100;
        if (!_set_field(&tmp, keys[i], vals[i])) return -100;
    }

    if (_is_blank(tmp.Fam) || _is_blank(tmp.Im)) return -2;

    bool key_changed = (contact_cmp(&tmp, &target->data) != 0);

    if (!key_changed) {
        target->data = tmp;
        if (out) *out = target;
        return 0;
    }

    TreeNode* new_node = NULL;
    int rc = tree_insert(g_tree, &tmp, &new_node);
    if (rc != 0) return rc;

    tree_remove(g_tree, target);
    if (out) *out = new_node;
    return 0;
}

/* ---------- удаление ---------- */

bool remove_contact(TreeNode* target)
{
    if (!g_tree || !target) return false;
    return tree_remove(g_tree, target);
}

/* ---------- снимок по префиксу фамилии ---------- */

struct Snapshot {
    TreeNode** items;
    size_t     count;
};

/* in-order обход поддерева, сбор совпадений по префиксу Fam */
static void _collect_prefix(TreeNode* n, const char* prefix, size_t plen,
    TreeNode** items, size_t* count, size_t cap, bool* stop)
{
    if (!n || *stop) return;

    int r = strncmp(n->data.Fam, prefix, plen);

    /* левое: обходить, только если n >= prefix (r >= 0);
       если r < 0 — всё левое < prefix → не совпадения → пропускаем */
    if (r >= 0)
        _collect_prefix(n->left, prefix, plen, items, count, cap, stop);
    if (*stop) return;

    if (r == 0) {
        if (*count < cap) items[(*count)++] = n;
        else { *stop = true; return; }
    }
    if (r > 0) {
        /* Fam > prefix — правое ещё больше → стоп */
        *stop = true;
        return;
    }

    /* r < 0 или r == 0 — идём вправо (там могут быть >= prefix) */
    _collect_prefix(n->right, prefix, plen, items, count, cap, stop);
}

Snapshot* snapshot_find_by_fam_prefix(const char* prefix)
{
    if (!g_tree || !prefix || !*prefix) return NULL;

    size_t cap = g_tree->size;
    if (cap == 0) return NULL;

    TreeNode** items = (TreeNode**)malloc(cap * sizeof(TreeNode*));
    if (!items) return NULL;

    size_t count = 0;
    bool   stop = false;
    _collect_prefix(g_tree->root, prefix, strlen(prefix), items, &count, cap, &stop);

    if (count == 0) { free(items); return NULL; }

    Snapshot* s = (Snapshot*)malloc(sizeof(Snapshot));
    if (!s) { free(items); return NULL; }
    s->items = items;
    s->count = count;
    return s;
}

size_t snapshot_count(const Snapshot* s) { return s ? s->count : 0; }

TreeNode* snapshot_get(const Snapshot* s, int k)
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