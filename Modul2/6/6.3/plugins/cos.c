#include "../plugin_common.h"

static double op_cos(double a) { return cos(a); }

static const Command cmd = {
    'C', CMD_UNARY, { .un = op_cos }, "косинус от a", 0, false
};

PLUGIN_EXPORT const Command* calc_plugin(void) { return &cmd; }