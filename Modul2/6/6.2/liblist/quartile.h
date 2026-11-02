#ifndef QUARTILE_H
#define QUARTILE_H

#include "list.h"

typedef struct { int wl; int wr; int thr; } QConfig;

extern const QConfig QCFG[3];

void  _qi_activate(List* list);
void  _qi_deactivate(List* list);
void  _qi_on_insert(List* list, Node* n);
void  _qi_on_remove(List* list, Node* n);
Node* _qi_find_cmp_start(List* list, const Contact* key);
Node* _qi_find_prefix_start(List* list, const char* prefix);

#endif