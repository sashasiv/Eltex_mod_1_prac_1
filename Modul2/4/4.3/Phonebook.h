#ifndef PHONEBOOK_H
#define PHONEBOOK_H

#define _CRT_SECURE_NO_WARNINGS
#include <stdbool.h>
#include "tree.h"

/* коды (единые):
     0    — успех
    -1    — дубликат
    -2    — пустые Fam/Im (только add/edit, до tree_insert)
    -100  — неизвестная ошибка (malloc и пр.) */

void phonebook_init(void);                          /* 8 предустановок */

size_t contacts_count(void);

/* обход в заданном порядке (LTR/TLR/LRT) */
void contacts_foreach(Order order, void (*fn)(const Contact*, void*), void* ctx);

/* вывод деревом (горизонтальный, с h/bf) */
void contacts_print_tree(void);

/* добавление: 0 / -1 (дубликат) / -2 (пусто) / -100 */
int add_contact(const char* fam, const char* im, ...);

/* поиск узла по ключу */
TreeNode* find_node_by_key(const Contact* key);

/* точный поиск: Contact* (data узла) или NULL */
Contact* find_by_key(const Contact* key);

/* правка: 0 / -1 / -2 / -100; *out — итоговый узел (при 0):
   тот же при несмене ключа, новый при смене. */
int edit_contact(TreeNode* target, TreeNode** out, ...);

int edit_contact_n(TreeNode* target, TreeNode** out,
    const char** keys, const char** vals, int n);

/* удаление по узлу */
bool remove_contact(TreeNode* target);

/* снимок по префиксу фамилии (только для выбора; destroy сразу) */
typedef struct Snapshot Snapshot;
Snapshot* snapshot_find_by_fam_prefix(const char* prefix);  /* NULL — нет */
size_t    snapshot_count(const Snapshot* s);
TreeNode* snapshot_get(const Snapshot* s, int k);           /* 1-based */
void      snapshot_destroy(Snapshot* s);

#endif