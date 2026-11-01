#ifndef CORE_H
#define CORE_H

#include <stddef.h>

/* результат разбора — код возврата */
typedef enum {
    PARSE_OK = 0,
    PARSE_ERR_FORMAT,   /* не тот формат/длина */
    PARSE_ERR_DIGIT,    /* не восьмеричная цифра */
    PARSE_ERR_CHAR      /* недопустимый символ */
} parse_status_t;

// Диапазон mode: 0..07777 (12 бит: SUID|SGID|sticky|owner|group|other).

/* Восьмеричная запись: необязательный ведущий '0' + 0..4 восьмеричные цифры.*/
parse_status_t mode_from_octal(const char* s, unsigned* mode_out);

/* Буквенная запись : 9 символов прав или 10 символов(тип файла + 9 прав) Допустимые символы прав: r w x - s S t T.*/
parse_status_t mode_from_symbolic(const char* s, unsigned* mode_out);

/* Автоопределение по первому символу: если все символы 0-7 -> octal, если тип файла или r symbolic */
parse_status_t mode_parse(const char* s, unsigned* mode_out);

/* форматирование mode -> строки (вызывающий даёт буферы) */
void mode_to_symbolic(unsigned mode, char out[10]);
void mode_to_octal(unsigned mode, char out[5]);
void mode_to_binary(unsigned mode, char out[16]);

const char* parse_status_str(parse_status_t st);

/* Применить chmod-подобную команду к base.
   Принимает:
     - chmod-команду: [ugoa][+-=][rwxst], через запятую;
     - восьмеричную запись: 1..4 цифры + необязательный '0' ("755", "0755", "4777").
   Для octal base игнорируется (замена маски); для chmod — base модифицируется. */
parse_status_t mode_apply_chmod(unsigned base, const char* cmd, unsigned* mode_out);

#endif /* CORE_H */