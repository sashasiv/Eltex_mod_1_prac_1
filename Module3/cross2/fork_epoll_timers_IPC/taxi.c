#include "taxi.h"

struct driver {
    pid_t pid;
    int   cmd_w;    /* parent -> driver */
    int   resp_r;   /* driver -> parent */
    int   alive;
};

static struct driver drivers[MAX_DRIVERS];
static int n_drivers = 0;
static pid_t self_pid;

static volatile sig_atomic_t g_stop = 0;

static void on_stop(int sig) {
    (void)sig;
    g_stop = 1;
}

/* ---- create_driver ---- */
static void cmd_create_driver(void) {
    if (n_drivers >= MAX_DRIVERS) { printf("Водителей слишком много!\n"); return; }

    int cmd_p[2], resp_p[2];
    if (pipe2(cmd_p, O_NONBLOCK) < 0) { perror("pipe2"); return; }
    if (pipe2(resp_p, O_NONBLOCK) < 0) { perror("pipe2"); return; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        close(cmd_p[1]);
        close(resp_p[0]);
        for (int i = 0; i < n_drivers; i++) {
            if (drivers[i].cmd_w >= 0) close(drivers[i].cmd_w);
            if (drivers[i].resp_r >= 0) close(drivers[i].resp_r);
        }
        char a1[16], a2[16];
        snprintf(a1, sizeof(a1), "%d", cmd_p[0]);
        snprintf(a2, sizeof(a2), "%d", resp_p[1]);
        execl("./driver", "./driver", a1, a2, (char*)NULL);
        perror("execl");
        _exit(1);
    }

    drivers[n_drivers].pid = pid;
    drivers[n_drivers].cmd_w = cmd_p[1];
    drivers[n_drivers].resp_r = resp_p[0];
    drivers[n_drivers].alive = 1;
    n_drivers++;

    printf("Driver created: pid=%d\n", pid);
}

static struct driver* find_driver(pid_t pid) {
    for (int i = 0; i < n_drivers; i++)
        if (drivers[i].pid == pid && drivers[i].alive) return &drivers[i];
    return NULL;
}

/* Синхронная команда: пишем в cmd_w, ждём resp через epoll с таймаутом */
static int transact(struct driver* d, struct msg* req, struct msg* resp, int to_ms) {
    if (full_write(d->cmd_w, req, sizeof(*req)) < 0) return -1;

    int ep = epoll_create1(0);
    struct epoll_event ev;
    ev.events = EPOLLIN; ev.data.fd = d->resp_r;
    epoll_ctl(ep, EPOLL_CTL_ADD, d->resp_r, &ev);

    int ok = -1;
    int n = epoll_wait(ep, &ev, 1, to_ms);
    if (n > 0) {
        ssize_t r = read(d->resp_r, resp, sizeof(*resp));
        if (r == (ssize_t)sizeof(*resp)) ok = 0;
    }
    close(ep);
    return ok;
}

static void cmd_send_task(pid_t pid, int sec) {
    struct driver* d = find_driver(pid);
    if (!d) { printf("У нас нет водителя: %d\n", pid); return; }

    struct msg req = { .type = 0, .cmd = CMD_SEND_TASK, .task_timer = sec };
    struct msg resp;
    if (transact(d, &req, &resp, 2000) < 0) {
        if (g_stop) return;             /* прервано сигналом — не шумим */
        printf("Водитель %d не отвечает\n", pid);
        d->alive = 0;
        return;
    }
    printf("Водитель %d: %s\n", pid, resp.text);
}

static void cmd_get_status(pid_t pid) {
    struct driver* d = find_driver(pid);
    if (!d) { printf("У нас нет водителя: %d\n", pid); return; }

    struct msg req = { .type = 0, .cmd = CMD_GET_STATUS };
    struct msg resp;
    if (transact(d, &req, &resp, 2000) < 0) {
        if (g_stop) return;             /* прервано сигналом — не шумим */
        printf("Водитель %d не отвечает\n", pid);
        d->alive = 0;
        return;
    }
    printf("Водитель %d: %s\n", pid, resp.text);
}

static void cmd_get_drivers(void) {
    printf("%-8s %-12s %s\n", "PID", "STATUS", "DETAIL");
    for (int i = 0; i < n_drivers; i++) {
        if (!drivers[i].alive) continue;
        struct msg req = { .type = 0, .cmd = CMD_GET_STATUS };
        struct msg resp;
        if (transact(&drivers[i], &req, &resp, 1000) < 0) {
            if (g_stop) return;
            printf("%-8d %-12s (не отвечает)\n", drivers[i].pid, "?");
            continue;
        }
        printf("%-8d %-12s %s\n", drivers[i].pid,
            resp.status == ST_BUSY ? "Занят" : "Свободен", resp.text);
    }
}

static void sigchld_handler(int sig) {
    (void)sig;
    int saved = errno, st;
    pid_t p;
    while ((p = waitpid(-1, &st, WNOHANG)) > 0) {
        for (int i = 0; i < n_drivers; i++) {
            if (drivers[i].pid == p) {
                drivers[i].alive = 0;
                close(drivers[i].cmd_w);
                close(drivers[i].resp_r);
                printf("\n[Водитель %d прекратил работу]\n", p);
            }
        }
    }
    errno = saved;
}

int main(void) {

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_stop;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;          /* SA_RESTART НЕ ставим — хотим, чтобы fgets прервался */
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    self_pid = getpid();
    signal(SIGCHLD, sigchld_handler);
    signal(SIGPIPE, SIG_IGN);

    printf("Станция такси. Команды:\n"
        "  create_driver\n"
        "  send_task <pid> <sec>\n"
        "  get_status <pid>\n"
        "  get_drivers\n"
        "  exit\n");

    char line[256];
    while (!g_stop) {
        printf("> "); fflush(stdout);
        errno = 0;
        if (!fgets(line, sizeof(line), stdin)) {
            if (g_stop)   break;      /* Ctrl+C / SIGTERM */
            if (feof(stdin)) break;   /* EOF */
            continue;
        }

        char cmd[64], a1[64], a2[64];
        int n = sscanf(line, "%63s %63s %63s", cmd, a1, a2);
        if (n <= 0) continue;

        if (!strcmp(cmd, "create_driver"))             cmd_create_driver();
        else if (!strcmp(cmd, "send_task") && n == 3) cmd_send_task((pid_t)atoi(a1), atoi(a2));
        else if (!strcmp(cmd, "get_status") && n == 2) cmd_get_status((pid_t)atoi(a1));
        else if (!strcmp(cmd, "get_drivers"))          cmd_get_drivers();
        else if (!strcmp(cmd, "exit"))                 break;
        else printf("Неизвестная команда\n");
    }

    /* закрытие cmd_w даёт драйверам EOF — они выходят сами */
    for (int i = 0; i < n_drivers; i++)
        if (drivers[i].alive && drivers[i].cmd_w >= 0)
            close(drivers[i].cmd_w);

    /* добить тех, кто не вышел по EOF */
    for (int i = 0; i < n_drivers; i++)
        if (drivers[i].alive)
            kill(drivers[i].pid, SIGTERM);

    /* reap всех */
    int st; pid_t p;
    while ((p = waitpid(-1, &st, 0)) > 0) {}

    /* закрыть оставшиеся fd */
    for (int i = 0; i < n_drivers; i++)
        if (drivers[i].resp_r >= 0) close(drivers[i].resp_r);
    
    return 0;
}