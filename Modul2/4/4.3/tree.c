#include "tree.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---------- компаратор ---------- */

int contact_cmp(const Contact* a, const Contact* b)
{
    int r;
    if ((r = strcmp(a->Fam, b->Fam)) != 0) return r;
    if ((r = strcmp(a->Im, b->Im)) != 0) return r;
    if ((r = strcmp(a->Otch, b->Otch)) != 0) return r;
    if ((r = strcmp(a->workplace, b->workplace)) != 0) return r;
    if ((r = strcmp(a->position, b->position)) != 0) return r;
    return 0;
}

/* ---------- высота ---------- */

int tree_height(const TreeNode* n) { return n ? n->height : -1; }

static int _max(int a, int b) { return a > b ? a : b; }

static void _update_height(TreeNode* n)
{
    if (n) n->height = 1 + _max(tree_height(n->left), tree_height(n->right));
}

static int _bf(const TreeNode* n)
{
    return n ? tree_height(n->left) - tree_height(n->right) : 0;
}

/* ---------- повороты ---------- */

/* правый поворот (LL) */
static TreeNode* _rotate_right(TreeNode* z)
{
    TreeNode* y = z->left;
    TreeNode* t2 = y->right;

    y->right = z;
    z->left = t2;

    _update_height(z);
    _update_height(y);
    return y;   /* новый корень поддерева */
}

/* левый поворот (RR) */
static TreeNode* _rotate_left(TreeNode* z)
{
    TreeNode* y = z->right;
    TreeNode* t2 = y->left;

    y->left = z;
    z->right = t2;

    _update_height(z);
    _update_height(y);
    return y;
}

/* балансировка поддерева с корнем z; возвращает новый корень */
static TreeNode* _balance(TreeNode* z)
{
    _update_height(z);
    int bf = _bf(z);

    if (bf > 1) {                       /* левый перевес */
        if (_bf(z->left) < 0)           /* LR */
            z->left = _rotate_left(z->left);
        return _rotate_right(z);        /* LL (или после LR) */
    }
    if (bf < -1) {                      /* правый перевес */
        if (_bf(z->right) > 0)          /* RL */
            z->right = _rotate_right(z->right);
        return _rotate_left(z);         /* RR (или после RL) */
    }
    return z;                           /* сбалансировано */
}

/* ---------- жизненный цикл ---------- */

Tree* tree_create(void)
{
    Tree* t = (Tree*)malloc(sizeof(Tree));
    if (!t) return NULL;
    t->root = NULL;
    t->size = 0;
    return t;
}

static void _destroy_node(TreeNode* n)
{
    if (!n) return;
    _destroy_node(n->left);
    _destroy_node(n->right);
    free(n);
}

void tree_destroy(Tree* t)
{
    if (!t) return;
    _destroy_node(t->root);
    free(t);
}

/* ---------- вставка (AVL) ---------- */

/* рекурсивная вставка; rc — код (0/-1/-100); *out — новый узел (если 0) */
static int _insert(TreeNode* n, const Contact* c, TreeNode** root_out, TreeNode** out)
{
    if (!n) {
        TreeNode* nn = (TreeNode*)malloc(sizeof(TreeNode));
        if (!nn) return -100;
        nn->data = *c;
        nn->left = nn->right = NULL;
        nn->height = 0;
        *root_out = nn;
        *out = nn;
        return 0;
    }

    int cmp = contact_cmp(c, &n->data);
    if (cmp == 0) return -1;   /* дубликат */

    int rc;
    if (cmp < 0) {
        rc = _insert(n->left, c, &n->left, out);
        if (rc != 0) return rc;
    }
    else {
        rc = _insert(n->right, c, &n->right, out);
        if (rc != 0) return rc;
    }

    *root_out = _balance(n);
    return 0;
}

int tree_insert(Tree* t, const Contact* c, TreeNode** out)
{
    if (out) *out = NULL;
    if (!t || !c) return -100;

    TreeNode* new_node = NULL;
    int rc = _insert(t->root, c, &t->root, &new_node);
    if (rc == 0) {
        t->size++;
        if (out) *out = new_node;
    }
    return rc;
}

/* ---------- удаление (AVL) ---------- */

/* найти минимум в поддереве (in-order successor) */
static TreeNode* _max_node(TreeNode* n)
{
    while (n && n->right) n = n->right;
    return n;
}

/* рекурсивное удаление по ключу; возвращает новый корень поддерева; *removed — удалённый узел (для free) */
static TreeNode* _remove_key(TreeNode* n, const Contact* key, TreeNode** removed)
{
    if (!n) return NULL;

    int cmp = contact_cmp(key, &n->data);
    if (cmp < 0) {
        n->left = _remove_key(n->left, key, removed);
    }
    else if (cmp > 0) {
        n->right = _remove_key(n->right, key, removed);
    }
    else {
        /* нашли */
        if (!n->left || !n->right) {
            TreeNode* child = n->left ? n->left : n->right;
            *removed = n;
            return child;                 /* child (или NULL) встаёт на место n */
        }
        else {
            /* два ребёнка: предшественник — максимум в левом поддереве */
            TreeNode* pred = _max_node(n->left);
            n->data = pred->data;
            n->left = _remove_key(n->left, &pred->data, removed);
            /* *removed — это pred (удалённый), не n */
        }
    }

    return _balance(n);
}

bool tree_remove(Tree* t, TreeNode* node)
{
    if (!t || !node) return false;

    TreeNode* removed = NULL;
    t->root = _remove_key(t->root, &node->data, &removed);

    if (removed) {
        free(removed);
        t->size--;
        return true;
    }
    return false;   /* ключ не найден (аномалия: node не из дерева) */
}

/* ---------- поиск ---------- */

TreeNode* tree_find(Tree* t, const Contact* key)
{
    if (!t || !key) return NULL;
    TreeNode* cur = t->root;
    while (cur) {
        int cmp = contact_cmp(key, &cur->data);
        if (cmp == 0) return cur;
        cur = (cmp < 0) ? cur->left : cur->right;
    }
    return NULL;
}

/* ---------- обходы ---------- */

static void _foreach(TreeNode* n, Order order,
    void (*fn)(const Contact*, void*), void* ctx)
{
    if (!n) return;
    if (order == ORDER_TLR) fn(&n->data, ctx);
    _foreach(n->left, order, fn, ctx);
    if (order == ORDER_LTR) fn(&n->data, ctx);
    _foreach(n->right, order, fn, ctx);
    if (order == ORDER_LRT) fn(&n->data, ctx);
}

void tree_foreach(TreeNode* root, Order order,
    void (*fn)(const Contact*, void*), void* ctx)
{
    if (!fn) return;
    _foreach(root, order, fn, ctx);
}

/* ---------- вывод деревом (горизонтальный) ---------- */

void tree_print(const TreeNode* n, const char* prefix, bool is_last, bool is_root)
{
    if (!n) return;

    /* печать текущего узла */
    if (is_root) {
        printf("%s %s [h=%d, bf=%d]\n", n->data.Fam, n->data.Im,
            n->height, _bf(n));
    }
    else {
        printf("%s%s%s %s [h=%d, bf=%d]\n", prefix,
            is_last ? "└─ " : "├─ ",
            n->data.Fam, n->data.Im, n->height, _bf(n));
    }

    /* префикс для детей */
    char child_prefix[256];
    if (is_root) snprintf(child_prefix, sizeof(child_prefix), "%s", prefix);
    else snprintf(child_prefix, sizeof(child_prefix), "%s%s",
        prefix, is_last ? "   " : "│  ");

    /* порядок: левое, правое (TLR-подобно: корень уже напечатан) */
    if (n->left || n->right) {
        if (n->left)  tree_print(n->left, child_prefix, n->right == NULL, false);
        if (n->right) tree_print(n->right, child_prefix, true, false);
    }
}