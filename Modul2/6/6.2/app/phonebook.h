#ifndef PHONEBOOK_H
#define PHONEBOOK_H

#include <stdbool.h>
#include "list.h"

/* коды (единые):
     0    — успех
    -1    — дубликат
    -2    — пустые Fam/Im
    -100  — неизвестная ошибка (malloc и пр.) */

/* --- инициализация (8 предустановок) --- */
void phonebook_init(void);

/* --- размер / обход --- */
size_t contacts_count(void);
void   contacts_foreach(void (*fn)(const Contact*, void*), void* ctx);

/* --- добавление: 0/-1/-2/-100  */
int add_contact(const char* fam, const char* im, ...);

/* --- поиск узла по точному ключу (для UI после add) --- */
Node* find_node_by_key(const Contact* key);

/* --- точный поиск: Contact* (data узла) или NULL --- */
Contact* find_by_key(const Contact* key);

/* --- правка: 0/-1/-2/-100; *out — итоговый узел (при 0) --- */
int edit_contact(Node* target, Node** out, ...);

/* правка по массиву пар: 0/-1/-2/-100; *out — итоговый узел */
int edit_contact_fields(Node* target, Node** out, const char** keys, const char** vals, int n);

/* --- удаление по узлу --- */
bool remove_contact(Node* target);

/* --- снимок результатов поиска по префиксу фамилии --- */
typedef struct Snapshot Snapshot;

Snapshot* snapshot_find_by_fam_prefix(const char* prefix);  /* NULL — нет совпадений */
size_t    snapshot_count(const Snapshot* s);
Node*     snapshot_get(const Snapshot* s, int k);           /* 1-based */
void      snapshot_destroy(Snapshot* s);                    /* единственный выход */

#endif