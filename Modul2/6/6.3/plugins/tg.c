#include "../plugin_common.h"

static double op_tg(double a) { return tan(a); }

static const Command cmd = {
    'T', CMD_UNARY, { .un = op_tg }, "тангенс от a", 0, false
};

PLUGIN_EXPORT const Command* calc_plugin(void) { return &cmd; }