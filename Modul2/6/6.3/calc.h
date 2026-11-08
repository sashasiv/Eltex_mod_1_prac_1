#ifndef CALC_H
#define CALC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

#define MAX_TOKENS   64
#define MAX_LINE     256
#define MAX_ERR      128
#define MAX_COMMANDS 64
#define MAX_PATH_LEN 512

typedef double (*bin_func)(double a, double b);
typedef double (*un_func)(double a);

typedef enum { CMD_BINARY, CMD_UNARY } CmdKind;

typedef struct {
    char    name;
    CmdKind kind;
    union { bin_func bin; un_func un; } fn;
    const char* descr;
    int         precedence;
    bool        right_assoc;
} Command;

typedef enum { TOK_NUMBER, TOK_OP, TOK_END } TokType;
typedef struct {
    TokType        type;
    double         num;
    const Command* cmd;
} Token;

void print_menu(const Command* cmds, int n);
int  read_line(char* buf, int size);

bool tokenize(const char* line,
    const Command* cmds, int n,
    Token* tokens, int max_tokens, int* out_count,
    char* errbuf, int errbuf_size);

bool parse_expression(const Token* tokens, int count,
    double* out_value,
    char* errbuf, int errbuf_size);

#endif