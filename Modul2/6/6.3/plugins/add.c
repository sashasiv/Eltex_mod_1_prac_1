#include "../plugin_common.h"

static double op_add(double a, double b) { return a + b; }

static const Command cmd = {
    '+', CMD_BINARY,{ .bin = op_add }, "сложение", 1, false
};

PLUGIN_EXPORT const Command* calc_plugin(void) { return &cmd; }