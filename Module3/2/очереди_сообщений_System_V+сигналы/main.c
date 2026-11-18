#include "broker.h"
#include "publisher.h"
#include "subscriber.h"
#include "common.h"

static void usage(const char* prog)
{
    fprintf(stderr,
        "использование:\n"
        "  %s -b                     — брокер\n"
        "  %s -p <топик>             — издатель\n"
        "  %s -s [<топик1> <топик2> ...] — подписчик\n",
        prog, prog, prog);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-b") == 0)
        return run_broker(argc, argv);

    if (strcmp(argv[1], "-p") == 0) {
        if (argc < 3) {
            fprintf(stderr, "издатель: не указан топик\n");
            usage(argv[0]);
            return 1;
        }
        return run_publisher(argc, argv);
    }

    if (strcmp(argv[1], "-s") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Предупреждение! подписчик: не указаны топики\n");
        }
        return run_subscriber(argc, argv);
    }

    fprintf(stderr, "неизвестный ключ: %s\n", argv[1]);
    usage(argv[0]);
    return 1;
}