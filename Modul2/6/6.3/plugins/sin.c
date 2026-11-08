#include "../plugin_common.h"

static double op_sin(double a) { return sin(a); }

static const Command cmd = {
    'S', CMD_UNARY, { .un = op_sin }, "синус от a", 0, false
};

PLUGIN_EXPORT const Command* calc_plugin(void) { return &cmd; }