#ifndef CHAT_H
#define CHAT_H

#include <pthread.h>
#include <netinet/in.h>

#define PORT        8080        /* В TCP используем другой порт */
#define BUF_SIZE    8192        /* Увеличим буфер для передачи файлов */
#define NAME_LEN    32
#define MAX_FILENAME 64

/* Префиксы протокола */
#define PFX_JOIN    "JOIN"
#define PFX_LEAVE   "LEAVE"
#define PFX_MSG     "MSG"
#define PFX_FILE    "FILE"      /* Новый тип для файлов */

#define MAX_CLIENTS 100

typedef enum {
    MSG_JOIN,
    MSG_LEAVE,
    MSG_CHAT,
    MSG_FILE,               /* Новый тип */
    MSG_INVALID
} msg_type_t;

/* Структура сообщения расширяется для файлов */
typedef struct {
    msg_type_t type;
    unsigned   id;
    char       name[NAME_LEN];
    char       text[BUF_SIZE];      /* Текст сообщения или имя файла */
    size_t     file_size;           /* Размер файла (для MSG_FILE) */
} message_t;

/* Контекст клиента (для TCP он немного другой) */
typedef struct {
    int                sock;
    unsigned           my_id;
    char               my_name[NAME_LEN];
    volatile int       running;
    pthread_mutex_t    print_mtx;
} client_t;

#endif