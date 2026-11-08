#include "../plugin_common.h"

static double op_sub(double a, double b) { return a - b; }

static const Command cmd = {
    '-', CMD_BINARY, { .bin = op_sub }, "вычитание", 1, false
};

PLUGIN_EXPORT const Command* calc_plugin(void) { return &cmd; }