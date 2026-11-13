// msg.h
#ifndef MSG_H
#define MSG_H

#include <stdint.h>

#define BLOCK_SIZE   4096

enum msg_type {
    MSG_READY = 1,  // ребёнок -> родитель: готов
    MSG_HEADER = 2,  // родитель -> ребёнок: имя + размер
    MSG_DATA = 3,  // родитель -> ребёнок: блок данных
    MSG_END = 4,  // родитель -> ребёнок: конец передачи
    MSG_DONE = 5,  // ребёнок -> родитель: файл записан
    // все прочие - UNKNOWN
};

struct msg {
    int32_t type;
    int32_t size;         // HEADER — размер файла; DATA — размер блока
    char    payload[BLOCK_SIZE];  // HEADER — имя файла; DATA — данные
};

// Человекочитаемое имя типа сообщения — для логов
const char* msg_type_name(int type);

#endif