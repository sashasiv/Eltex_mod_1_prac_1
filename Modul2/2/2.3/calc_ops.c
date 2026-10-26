#include "calc.h"

/* ---------- Обработчики ----------
   Контракт bin_func: возвращают значение; ошибка — через NaN/Inf.
   Проверка isfinite() — на стороне parse_expression. */

static double op_add(double a, double b) { return a + b; }
static double op_sub(double a, double b) { return a - b; }
static double op_mul(double a, double b) { return a * b; }
static double op_div(double a, double b) { return a / b; } // деление на 0 детектится через Inf
static double op_pow(double a, double b) { return pow(a, b); }
static double op_log(double a, double b) { return log(a) / log(b); }
static double op_sin(double a) { return sin(a); }

/* ---------- Таблица команд ----------
   Инварианты (проверяются validate_commands() при старте, см. main.c):
   - name: ровно 1 символ; уникален; не зарезервирован
           Зарезервированы: ('q','Q' — выход; '(',')' — скобки; '.', цифры; пробел);
   - kind: тип функции на данный момент CMD_BINARY - бинарная (NUM OP NUM) CMD_UNARY - унарная префиксная (OP NUM)
   - fn:  fn.bin != NULL для BINARY; fn.un != NULL для UNARY
   - descr: Описание != NULL;
   - precedence: Приоритет > 0 (больше = выше) для унарных всегда 0;
   - right_assoc: правоассоциативность (для '^'). для унарных всегда false 
*/

static const Command commands[] = {
    { '+', CMD_BINARY,  { .bin = op_add },  "сложение",                     1, false },
    { '-', CMD_BINARY,  { .bin = op_sub },  "вычитание",                    1, false },
    { '*', CMD_BINARY,  { .bin = op_mul },  "умножение",                    2, false },
    { '/', CMD_BINARY,  { .bin = op_div },  "деление",                      2, false },
    { '^', CMD_BINARY,  { .bin = op_pow },  "возведение в степень",         3, true  },
    { 'L', CMD_BINARY,  { .bin = op_log },  "логарифм a по основанию b",    2, false },
    { 'S', CMD_UNARY,   { .un = op_sin  },  "синус от а",                   0, false },
};

const Command* get_commands(int* count) {
    *count = (int)(sizeof(commands) / sizeof(commands[0]));
    return commands;
}