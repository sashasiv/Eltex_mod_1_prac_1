#include "calc.h"

static const Command* find_op(char c, const Command* cmds, int n) {
    for (int i = 0; i < n; i++)
        if (cmds[i].name == c) return &cmds[i];
    return NULL;
}

bool tokenize(const char* line,
    const Command* cmds, int n,
    Token* tokens, int max_tokens, int* out_count,
    char* errbuf, int errbuf_size)
{
    int count = 0;
    const char* p = line;

    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        if (count >= max_tokens - 1) {
            snprintf(errbuf, errbuf_size, "слишком много токенов");
            return false;
        }

        char* end = NULL;
        double v = strtod(p, &end);
        if (end != p) {
            tokens[count++] = (Token){ .type = TOK_NUMBER, .num = v, .cmd = NULL };
            p = end;
            continue;
        }

        const Command* cmd = find_op(*p, cmds, n);
        if (cmd) {
            tokens[count++] = (Token){ .type = TOK_OP, .num = 0, .cmd = cmd };
            p++;
            continue;
        }

        snprintf(errbuf, errbuf_size, "неизвестный символ '%c'", *p);
        return false;
    }

    tokens[count++] = (Token){ .type = TOK_END, .num = 0, .cmd = NULL };
    *out_count = count;
    return true;
}