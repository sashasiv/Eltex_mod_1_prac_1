/* ============================================================
 * message.h — форма сообщений
 * Формат вывода :
* [you]    <текст>   — печатает sender
* [peer]   <текст>   — печатает receiver
* [system] <событие> — печатает main / receiver
* ============================================================ */
#ifndef MESSAGE_H
#define MESSAGE_H

#include "config.h"

struct chat_msg {
    char text[MSG_MAX];   /* всегда с '\0' на конце (обнуляем перед send) */
};

#endif /* MESSAGE_H */