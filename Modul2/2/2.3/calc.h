#ifndef CALC_H
#define CALC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

#define MAX_TOKENS 64 // пока используется только 3-4
#define MAX_LINE   256
#define MAX_ERR    128

/* ---------- Контракт обработчика ---------- */
typedef double (*bin_func)(double a, double b);
typedef double (*un_func)(double a);

typedef enum { CMD_BINARY, CMD_UNARY } CmdKind;

typedef struct {
    char    name;
    CmdKind kind;
    union {
        bin_func bin;
        un_func un;
    } fn;
    const char* descr;
    int         precedence;   // только для CMD_BINARY при CMD_UNARY = 0
    bool        right_assoc;  // только для CMD_BINARY при CMD_UNARY = false
} Command;

const Command* get_commands(int* count);

/* ---------- Токены (задел под парсер выражений) ---------- */
typedef enum { TOK_NUMBER, TOK_OP, TOK_END } TokType;

typedef struct {
    TokType        type;
    double         num;   /* TOK_NUMBER */
    const Command* cmd;   /* TOK_OP */
} Token;

/* ---------- Приём/выдача ---------- */
void print_menu(const Command* cmds, int n);
int  read_line(char* buf, int size);

/* ---------- Разбор ---------- */
/* Токенизация: строка -> массив токенов. true = успех, false = ошибка (errbuf заполнен). */
bool tokenize(const char* line,
    const Command* cmds, int n,
    Token* tokens, int max_tokens, int* out_count,
    char* errbuf, int errbuf_size);

/* Разбор: токены -> значение. true = успех, false = ошибка (errbuf заполнен).
   Сейчас поддерживает ровно NUMBER BIN_OP NUMBER END и UN_OP NUMBER END.
   Позже тело заменится на shunting-yard / рекурсивный спуск — сигнатура та же. */
bool parse_expression(const Token* tokens, int count,
    double* out_value,
    char* errbuf, int errbuf_size);

#endif