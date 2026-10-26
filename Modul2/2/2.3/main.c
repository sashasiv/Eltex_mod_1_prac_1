#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "calc.h"

/* ==================== Приём/выдача ==================== */

void print_menu(const Command* cmds, int n) {
    printf("\n=== КАЛЬКУЛЯТОР ===\n");
    for (int i = 0; i < n; i++) {
        if (cmds[i].kind == CMD_BINARY)
            printf("  a %c b  — %s\n", cmds[i].name, cmds[i].descr);
        else
            printf("  %c a    — %s\n", cmds[i].name, cmds[i].descr);
    }
    printf("  q      — выход\n");
}

int read_line(char* buf, int size) {
    if (!fgets(buf, size, stdin)) return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    return 1;
}

/* ==================== Токенизация ==================== */

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

        if (count >= max_tokens - 1) {   /* -1: место под END */
            snprintf(errbuf, errbuf_size, "слишком много токенов");
            return false;
        }

        /* 1) число (strtod первым — съест nan/inf целиком, если появятся) */
        char* end = NULL;
        double v = strtod(p, &end);
        if (end != p) {
            tokens[count++] = (Token){ .type = TOK_NUMBER, .num = v, .cmd = NULL };
            p = end;
            continue;
        }

        /* 2) операция */
        const Command* cmd = find_op(*p, cmds, n);
        if (cmd) {
            tokens[count++] = (Token){ .type = TOK_OP, .num = 0, .cmd = cmd };
            p++;
            continue;
        }

        /* 3) ни число, ни операция */
        snprintf(errbuf, errbuf_size, "неизвестный символ '%c'", *p);
        return false;
    }

    tokens[count++] = (Token){ .type = TOK_END, .num = 0, .cmd = NULL };
    *out_count = count;
    return true;
}

/* ==================== Разбор ==================== */

bool parse_expression(const Token* tokens, int count,
    double* out_value,
    char* errbuf, int errbuf_size)
{
    if (count < 1) {
        snprintf(errbuf, errbuf_size, "пустой ввод");
        return false;
    }

    /* ---- ветка BINARY: NUMBER OP NUMBER END ---- */
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

    /* ---- ветка UNARY: OP NUMBER END ---- */
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

/* ==================== main ==================== */

int main(void) {

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    int n = 0;
    const Command* cmds = get_commands(&n);

    char line[MAX_LINE];
    Token tokens[MAX_TOKENS];
    char err[MAX_ERR];

    printf("Калькулятор. Команды:");
    for (int i = 0; i < n; i++) printf(" %c", cmds[i].name);
    printf("\n");
    print_menu(cmds, n);

    for (;;) {
        printf("Введите выражение или q: ");
        if (!read_line(line, sizeof line)) break;

        if (line[0] == 'q' || line[0] == 'Q') break;
        if (line[0] == '\0') continue;

        int tc = 0;
        double value = 0;
        err[0] = '\0';

        if (!tokenize(line, cmds, n, tokens, MAX_TOKENS, &tc, err, sizeof err)) {
            printf("Ошибка: %s\n", err);
            continue;
        }
        if (!parse_expression(tokens, tc, &value, err, sizeof err)) {
            printf("Ошибка: %s\n", err);
            continue;
        }
        printf("Результат: %g\n", value);
    }

    printf("До свидания!\n");
    return 0;
}