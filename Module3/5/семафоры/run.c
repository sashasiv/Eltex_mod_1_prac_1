#include "config.h"
#include "shm_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

/*
 * Запускает producer + N consumers, ждёт всех, чистит ресурсы.
 * Использование: ./run [N]   (по умолчанию N=2)
 *
 * Ресурсы создаёт producer (SHM_ROLE_CREATE). run НЕ создаёт их сам —
 * иначе пришлось бы дублировать логику инициализации. Но run удаляет
 * их в конце (unlink_objects=1), т.к. он «владелец» запуска.
 */

static pid_t spawn(char* const argv[])
{
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return -1; }
    if (pid == 0) {
        execv(argv[0], argv);
        perror("execv");
        _exit(127);
    }
    return pid;
}

int main(int argc, char** argv)
{
    int n_consumers = (argc > 1) ? atoi(argv[1]) : 2;
    if (n_consumers < 1) n_consumers = 1;

    /* Абсолютные пути к бинарникам — рядом с run.
     * В учебной задаче проще передать через argv или скомпилировать
     * в один каталог и использовать "./producer". */
    char* prod_argv[] = { (char*)"./producer", NULL };
    char* cons_argv[] = { (char*)"./consumer", NULL };

    pid_t prod = spawn(prod_argv);
    if (prod < 0) return CFG_EXIT_RUNTIME;

    pid_t* cons = calloc((size_t)n_consumers, sizeof(pid_t));
    if (!cons) { kill(prod, SIGTERM); return CFG_EXIT_RUNTIME; }

    for (int i = 0; i < n_consumers; i++) {
        cons[i] = spawn(cons_argv);
        if (cons[i] < 0) {
            /* Убьём уже запущенных и выйдем. */
            kill(prod, SIGTERM);
            for (int j = 0; j < i; j++) kill(cons[j], SIGTERM);
            free(cons);
            return CFG_EXIT_RUNTIME;
        }
    }

    /* Ждём producer. */
    int status;
    waitpid(prod, &status, 0);
    CFG_LOG("run: producer завершился (status=%d)\n", status);

    /* Ждём потребителей. Они завершатся сами, когда увидят done+пусто. */
    for (int i = 0; i < n_consumers; i++) {
        waitpid(cons[i], &status, 0);
    }

    /* Чистим ресурсы. К этому моменту все процессы отсоединились,
     * так что unlink безопасен. */
    ShmList* l = shml_open(SHM_ROLE_ATTACH);
    if (l) {
        shml_close(l, 1);   /* unlink_objects = 1 */
        CFG_LOG("run: ресурсы удалены\n");
    }
    else {
        CFG_LOG("run: не удалось подключиться для очистки\n");
    }

    free(cons);
    return CFG_EXIT_OK;
}