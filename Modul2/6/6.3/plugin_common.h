#ifndef PLUGIN_COMMON_H
#define PLUGIN_COMMON_H

#include "calc.h"

#define PLUGIN_ENTRY "calc_plugin"

typedef const Command* (*plugin_getter)(void);

#define PLUGIN_EXPORT __attribute__((visibility("default")))

#endif