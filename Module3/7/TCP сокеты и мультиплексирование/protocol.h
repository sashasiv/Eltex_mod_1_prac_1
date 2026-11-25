#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "chat.h"
#include <stddef.h>

/* Сформировать строку сообщения в buf (размер sz).
 * Возврат: длина строки (без '\0') или -1, если не поместилось. */
int proto_make_join(char* buf, size_t sz, unsigned id, const char* name);
int proto_make_leave(char* buf, size_t sz, unsigned id, const char* name);
int proto_make_msg(char* buf, size_t sz, unsigned id,
    const char* name, const char* text);
int proto_make_file(char* buf, size_t sz, unsigned id, const char* name, const char* filename, size_t file_size);

/* Разобрать входящую строку в out.
 * Возврат: 0 при успехе, -1 при ошибке (out->type = MSG_INVALID). */
int proto_parse(const char* buf, size_t len, message_t* out);

#endif /* PROTOCOL_H */