#include "loader.h"
#include "plugin_common.h"

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>

#define LIB_EXT ".so"

static bool has_ext(const char* name, const char* ext) {
    size_t ln = strlen(name), le = strlen(ext);
    return ln > le && strcmp(name + ln - le, ext) == 0;
}

static void join_path(char* out, size_t sz, const char* dir, const char* name) {
    snprintf(out, sz, "%s/%s", dir, name);
}

static bool validate_command(const Command* c, const Command* existing, int n,
    char* errbuf, int errbuf_size)
{
    if (!c) { snprintf(errbuf, errbuf_size, "плагин вернул NULL"); return false; }
    if (c->name == '\0') { snprintf(errbuf, errbuf_size, "пустое имя команды"); return false; }
    if (!c->descr) { snprintf(errbuf, errbuf_size, "нет описания у '%c'", c->name); return false; }

    if (c->kind == CMD_BINARY) {
        if (!c->fn.bin) { snprintf(errbuf, errbuf_size, "'%c': NULL bin_func", c->name); return false; }
        if (c->precedence <= 0) { snprintf(errbuf, errbuf_size, "'%c': precedence <= 0", c->name); return false; }
    }
    else {
        if (!c->fn.un) { snprintf(errbuf, errbuf_size, "'%c': NULL un_func", c->name); return false; }
        if (c->precedence != 0) { snprintf(errbuf, errbuf_size, "'%c': унарная с precedence != 0", c->name); return false; }
        if (c->right_assoc) { snprintf(errbuf, errbuf_size, "'%c': унарная с right_assoc", c->name); return false; }
    }

    const char* reserved = "qQ().";
    if (strchr(reserved, c->name) || isdigit((unsigned char)c->name) || isspace((unsigned char)c->name)) {
        snprintf(errbuf, errbuf_size, "'%c': зарезервированный символ", c->name);
        return false;
    }

    for (int i = 0; i < n; i++) {
        if (existing[i].name == c->name) {
            snprintf(errbuf, errbuf_size, "дубликат команды '%c'", c->name);
            return false;
        }
    }
    return true;
}

int count_plugins(const char* dir) {
    DIR* d = opendir(dir);
    if (!d) return 0;
    struct dirent* ent;
    int count = 0;
    while ((ent = readdir(d)) != NULL) {
        if (has_ext(ent->d_name, LIB_EXT)) count++;
    }
    closedir(d);
    return count;
}

bool load_plugins(const char* dir,
    Command* cmds, int* n_cmds,
    LoadedPlugin* plugins, int max_plugins,
    int* n_plugins,
    char* errbuf, int errbuf_size)
{
    *n_cmds = 0;
    *n_plugins = 0;

    DIR* d = opendir(dir);
    if (!d) {
        snprintf(errbuf, errbuf_size, "не открыть каталог '%s': %s", dir, strerror(errno));
        return false;
    }

    struct dirent* ent;
    while ((ent = readdir(d)) != NULL) {
        if (!has_ext(ent->d_name, LIB_EXT)) continue;

        if (*n_plugins >= max_plugins) {
            snprintf(errbuf, errbuf_size, "слишком много плагинов (max %d)", max_plugins);
            closedir(d);
            unload_plugins(plugins, *n_plugins);
            return false;
        }

        char path[MAX_PATH_LEN];
        join_path(path, sizeof path, dir, ent->d_name);

        void* h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (!h) {
            fprintf(stderr, "пропуск '%s': %s\n", path, dlerror());
            continue;
        }

        plugin_getter getter = (plugin_getter)dlsym(h, PLUGIN_ENTRY);
        if (!getter) {
            fprintf(stderr, "пропуск '%s': нет символа '%s'\n", path, PLUGIN_ENTRY);
            dlclose(h);
            continue;
        }

        const Command* c = getter();
        if (!validate_command(c, cmds, *n_cmds, errbuf, errbuf_size)){
            fprintf(stderr, "пропуск '%s': %s\n", path, errbuf);
            dlclose(h);
            continue;
        }

        plugins[*n_plugins].handle = h;
        plugins[*n_plugins].cmd = *c;
        snprintf(plugins[*n_plugins].path, MAX_PATH_LEN, "%s", path);
        (*n_plugins)++;

        cmds[(*n_cmds)++] = *c;
    }
    closedir(d);

    if (*n_cmds == 0) {
        snprintf(errbuf, errbuf_size, "в каталоге '%s' не найдено ни одного плагина", dir);
        return false;
    }
    return true;
}

void unload_plugins(LoadedPlugin* plugins, int n) {
    for (int i = 0; i < n; i++)
        if (plugins[i].handle) dlclose(plugins[i].handle);
}