#include "../plugin_common.h"

static double op_pow(double a, double b) { return pow(a, b); }

static const Command cmd = {
    '^', CMD_BINARY, { .bin = op_pow }, "возведение в степень", 3, true
};

PLUGIN_EXPORT const Command* calc_plugin(void) { return &cmd; }