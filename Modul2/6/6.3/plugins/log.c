#include "../plugin_common.h"

/* log_a(b). Ошибки (a<=0, b<=0, b==1) дают NaN/±inf — ловятся в parse_expression */
static double op_log(double a, double b) { return log(a) / log(b); }

static const Command cmd = {
    'L', CMD_BINARY, { .bin = op_log }, "логарифм a по основанию b", 2, false
};

PLUGIN_EXPORT const Command* calc_plugin(void) { return &cmd; }