#ifndef TREE_H
#define TREE_H

#include <stdbool.h>
#include <stddef.h>
#include "contact.h"

/* коды возврата tree_insert / phonebook (единые):
     0    — успех
    -1    — дубликат (ключ уже есть; валидное поведение)
    -2    — пустые Fam/Im (только phonebook, до tree_insert; tree_insert не возвращает)
    -100  — неизвестная ошибка (malloc fail, NULL-аргументы и пр.) */

typedef struct TreeNode {
    Contact         data;
    struct TreeNode* left;
    struct TreeNode* right;
    int              height;     /* height(NULL) = -1, height(лист) = 0 */
} TreeNode;

typedef struct Tree {
    TreeNode* root;
    size_t    size;
} Tree;

/* порядок обхода */
typedef enum {
    ORDER_LTR = 0,   /* in-order: left-root-right (по умолчанию) */
    ORDER_TLR,       /* pre-order: root-left-right */
    ORDER_LRT        /* post-order: left-right-root */
} Order;

/* --- компаратор по ключу (Fam→Im→Otch→workplace→position) --- */
int contact_cmp(const Contact* a, const Contact* b);

/* --- жизненный цикл --- */
Tree* tree_create(void);
void  tree_destroy(Tree* t);

/* --- вставка ---
   0 успех (*out — новый узел, если out != NULL);
  -1 дубликат (валидно);
  -100 ошибка (malloc, t/c NULL).
   Возвращает -2 никогда (пустые Fam/Im — уровень phonebook). */
int tree_insert(Tree* t, const Contact* c, TreeNode** out);

/* --- удаление по указателю на узел (узел должен быть в дереве) --- */
bool tree_remove(Tree* t, TreeNode* node);

/* --- поиск по ключу --- */
TreeNode* tree_find(Tree* t, const Contact* key);

/* --- обход в заданном порядке --- */
void tree_foreach(TreeNode* root, Order order,
    void (*fn)(const Contact*, void*), void* ctx);

/* --- вывод деревом (горизонтальный, с h/bf) ---
   Вызывать: tree_print(root, "", true, true). */
void tree_print(const TreeNode* n, const char* prefix,
    bool is_last, bool is_root);

/* --- высота / баланс-фактор (для вывода/отладки) --- */
int tree_height(const TreeNode* n);   /* height(NULL) = -1 */

#endif