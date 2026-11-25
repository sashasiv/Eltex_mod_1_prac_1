/* ============================================================
 *  test_proto.c — тесты парсера протокола (отдельный бинарник).
 *  Сборка: make test_proto && ./test_proto
 * ============================================================ */

#include "protocol.h"
#include "chat.h"

#include <stdio.h>
#include <string.h>

static int g_total = 0;
static int g_failed = 0;

#define CHECK(cond, msg) do {                                   \
    g_total++;                                                  \
    if (!(cond)) {                                              \
        g_failed++;                                             \
        printf("  FAIL: %s  (%s:%d)\n", msg, __FILE__, __LINE__); \
    }                                                           \
} while (0)

/* ---------- Round-trip: make -> parse -> поля совпали ---------- */

static void test_roundtrip_msg(void)
{
    printf("[roundtrip MSG]\n");
    char buf[BUF_SIZE];
    int n = proto_make_msg(buf, sizeof(buf), 123, "Alice", "привет: как дела?");
    CHECK(n > 0, "make_msg вернул > 0");

    message_t m;
    CHECK(proto_parse(buf, (size_t)n, &m) == 0, "parse вернул 0");
    CHECK(m.type == MSG_CHAT, "type == MSG_CHAT");
    CHECK(m.id == 123, "id == 123");
    CHECK(strcmp(m.name, "Alice") == 0, "name == Alice");
    CHECK(strcmp(m.text, "привет: как дела?") == 0, "text с двоеточием сохранён");
}

static void test_roundtrip_join_leave(void)
{
    printf("[roundtrip JOIN/LEAVE]\n");
    char buf[BUF_SIZE];
    message_t m;

    int n = proto_make_join(buf, sizeof(buf), 42, "Bob");
    CHECK(n > 0 && proto_parse(buf, (size_t)n, &m) == 0, "JOIN parse");
    CHECK(m.type == MSG_JOIN && m.id == 42, "JOIN type/id");
    CHECK(strcmp(m.name, "Bob") == 0, "JOIN name");

    n = proto_make_leave(buf, sizeof(buf), 42, "Bob");
    CHECK(n > 0 && proto_parse(buf, (size_t)n, &m) == 0, "LEAVE parse");
    CHECK(m.type == MSG_LEAVE && m.id == 42, "LEAVE type/id");
    CHECK(strcmp(m.name, "Bob") == 0, "LEAVE name");
}

/* ---------- Спецсимволы в полях ---------- */

static void test_special_chars(void)
{
    printf("[спецсимволы]\n");
    char buf[BUF_SIZE];
    message_t m;

    /* двоеточие и | в имени и тексте */
    int n = proto_make_msg(buf, sizeof(buf), 1, "A:B", "x|y:z");
    CHECK(n > 0 && proto_parse(buf, (size_t)n, &m) == 0, "parse");
    CHECK(strcmp(m.name, "A:B") == 0, "name с ':' и '|'");
    CHECK(strcmp(m.text, "x|y:z") == 0, "text с ':' и '|'");

    /* пробелы и переводы строк в тексте */
    n = proto_make_msg(buf, sizeof(buf), 2, "N", "a b\tc");
    CHECK(n > 0 && proto_parse(buf, (size_t)n, &m) == 0, "parse с пробелами");
    CHECK(strcmp(m.text, "a b\tc") == 0, "text с пробелами");

    /* UTF-8 кириллица */
    n = proto_make_msg(buf, sizeof(buf), 3, "Иван", "Привет, мир!");
    CHECK(n > 0 && proto_parse(buf, (size_t)n, &m) == 0, "UTF-8 parse");
    CHECK(strcmp(m.name, "Иван") == 0, "UTF-8 name");
    CHECK(strcmp(m.text, "Привет, мир!") == 0, "UTF-8 text");
}

/* ---------- Битые входы: parse должен вернуть -1 ---------- */

static void expect_fail(const char* s, const char* msg)
{
    message_t m;
    int r = proto_parse(s, strlen(s), &m);
    CHECK(r == -1, msg);
    CHECK(m.type == MSG_INVALID, "type == MSG_INVALID после ошибки");
}

static void test_malformed(void)
{
    printf("[битые входы]\n");
    expect_fail("MSG|3|123|5|Alice|20|коротко", "длина text больше данных");
    expect_fail("MSG|abc|123|5|Alice|5|hello", "id не число");
    expect_fail("MSG|3|123|5|Alice", "MSG без text");
    expect_fail("JOIN|3|123|0|", "пустое имя");
    expect_fail("FOO|3|123|5|Alice", "неизвестный префикс");
    expect_fail("мусор", "не формат");
    expect_fail("", "пустая строка");
    expect_fail("MSG", "только префикс, нет |");
    expect_fail("MSG|", "нет полей");
    expect_fail("MSG|3|123|5|Alice|99999|x", "len text = 99999");
    expect_fail("JOIN|99999999999999999999|1|A", "переполнение длины");
}

/* ---------- Границы буфера ---------- */

static void test_buffer_bounds(void)
{
    printf("[границы буфера]\n");

    /* make в слишком маленький буфер -> -1 */
    char small[10];
    int n = proto_make_msg(small, sizeof(small), 123, "Alice", "hello world");
    CHECK(n == -1, "make_msg в маленький буфер -> -1");

    /* пустой текст — допустим */
    char buf[BUF_SIZE];
    n = proto_make_msg(buf, sizeof(buf), 1, "A", "");
    message_t m;
    CHECK(n > 0 && proto_parse(buf, (size_t)n, &m) == 0, "пустой text parse");
    CHECK(strcmp(m.text, "") == 0, "пустой text");
}

/* ---------- main ---------- */

int main(void)
{
    printf("=== тесты парсера протокола ===\n");
    test_roundtrip_msg();
    test_roundtrip_join_leave();
    test_special_chars();
    test_malformed();
    test_buffer_bounds();

    printf("\n=== итог: %d/%d пройдено, %d провалено ===\n",
        g_total - g_failed, g_total, g_failed);
    return g_failed ? 1 : 0;
}