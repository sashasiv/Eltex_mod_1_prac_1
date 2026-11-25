#ifndef CHAT_H
#define CHAT_H

#include <pthread.h>
#include <netinet/in.h>

 /* ---------- Сетевые параметры ---------- */
#define PORT        5000        /* UDP-порт чата (одинаков у всех) */
#define BUF_SIZE    1024        /* размер буфера сообщения         */
#define NAME_LEN    32          /* максимальная длина имени        */

/* ---------- Префиксы протокола ---------- */
#define PFX_JOIN    "JOIN"
#define PFX_LEAVE   "LEAVE"
#define PFX_MSG     "MSG"

/* ---------- Типы сообщений ---------- */
typedef enum {
    MSG_JOIN,       /* участник вошёл в сеть   */
    MSG_LEAVE,      /* участник покинул сеть   */
    MSG_CHAT,       /* обычное сообщение чата  */
    MSG_INVALID     /* нераспознанное сообщение */
} msg_type_t;

/* ---------- Разобранное входящее сообщение ---------- */
typedef struct {
    msg_type_t type;
    unsigned   id;
    char       name[NAME_LEN];
    char       text[BUF_SIZE];
} message_t;

/* ---------- Контекст клиента ----------
 * Единая структура, разделяемая всеми модулями.
 * Передаётся по указателю в поток приёма и в главный цикл.
 */
typedef struct {
    int                sock;        /* UDP-сокет                  */
    unsigned           my_id;       /* уникальный id клиента      */
    char               my_name[NAME_LEN];
    struct sockaddr_in bcast;       /* адрес broadcast-рассылки   */
    volatile int       running;     /* флаг работы (0 = выход)    */
    pthread_mutex_t    print_mtx;   /* защита вывода в консоль    */
} client_t;

#endif /* CHAT_H */