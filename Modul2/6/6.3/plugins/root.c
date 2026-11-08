#include "../plugin_common.h"

/* Корень b-й степени из a. Для a < 0 допустимо только нечётное целое b. */
static double op_root(double a, double b) {
    if (b <= 0) return NAN;
    if (a < 0) {
        if (floor(b) != b || fmod(b, 2.0) == 0.0) return NAN;
        return -pow(-a, 1.0 / b);
    }
    return pow(a, 1.0 / b);
}

static const Command cmd = {
    'R', CMD_BINARY, { .bin = op_root }, "корень b-й степени из a", 3, true
};

PLUGIN_EXPORT const Command* calc_plugin(void) { return &cmd; }