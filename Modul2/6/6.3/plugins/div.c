#include "../plugin_common.h"

/* Деление на 0 даёт ±inf или NaN — ловится в parse_expression через isfinite */
static double op_div(double a, double b) { return a / b; }

static const Command cmd = {
    '/', CMD_BINARY, { .bin = op_div }, "деление", 2, false
};

PLUGIN_EXPORT const Command* calc_plugin(void) { return &cmd; }