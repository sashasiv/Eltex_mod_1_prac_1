#include "calc.h"

bool parse_expression(const Token* tokens, int count,
    double* out_value,
    char* errbuf, int errbuf_size)
{
    if (count < 1) {
        snprintf(errbuf, errbuf_size, "пустой ввод");
        return false;
    }

    if (count == 4 &&
        tokens[0].type == TOK_NUMBER &&
        tokens[1].type == TOK_OP &&
        tokens[2].type == TOK_NUMBER &&
        tokens[3].type == TOK_END)
    {
        const Command* cmd = tokens[1].cmd;
        if (cmd->kind != CMD_BINARY) {
            snprintf(errbuf, errbuf_size, "'%c' — унарная, нужен вид '%c a'", cmd->name, cmd->name);
            return false;
        }
        double res = cmd->fn.bin(tokens[0].num, tokens[2].num);
        if (!isfinite(res)) {
            snprintf(errbuf, errbuf_size, "ошибка вычисления (%c)", cmd->name);
            return false;
        }
        *out_value = res;
        return true;
    }

    if (count == 3 &&
        tokens[0].type == TOK_OP &&
        tokens[1].type == TOK_NUMBER &&
        tokens[2].type == TOK_END)
    {
        const Command* cmd = tokens[0].cmd;
        if (cmd->kind != CMD_UNARY) {
            snprintf(errbuf, errbuf_size, "'%c' — бинарная, нужен вид 'a %c b'", cmd->name, cmd->name);
            return false;
        }
        double res = cmd->fn.un(tokens[1].num);
        if (!isfinite(res)) {
            snprintf(errbuf, errbuf_size, "ошибка вычисления (%c)", cmd->name);
            return false;
        }
        *out_value = res;
        return true;
    }

    snprintf(errbuf, errbuf_size, "неверная форма выражения");
    return false;
}