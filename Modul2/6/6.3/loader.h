#ifndef LOADER_H
#define LOADER_H

#include "calc.h"

typedef struct {
    void*   handle;
    Command cmd;          
    char    path[MAX_PATH_LEN];
} LoadedPlugin;

int count_plugins(const char* dir);

bool load_plugins(const char* dir,
    Command* cmds, int* n_cmds,
    LoadedPlugin* plugins, int max_plugins,
    int* n_plugins,
    char* errbuf, int errbuf_size);

void unload_plugins(LoadedPlugin* plugins, int n);

#endif