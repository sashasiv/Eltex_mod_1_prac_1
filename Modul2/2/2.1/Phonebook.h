#define _CRT_SECURE_NO_WARNINGS
#ifndef PHONEBOOK_H
#define PHONEBOOK_H


#include <stdbool.h>
#include <string.h>

#define MAX_CONTACTS 100
#define STR_LEN      64
#define ARR_LEN      9

typedef struct {
    int  id;
    char Fam[STR_LEN];        /* обязательно */
    char Im[STR_LEN];           /* обязательно */
    char Otch[STR_LEN];
    char workplace[STR_LEN];
    char position[STR_LEN];
    char phones[ARR_LEN][STR_LEN];
    int  phones_count;
    char emails[ARR_LEN][STR_LEN];
    int  emails_count;
    char socials[ARR_LEN][STR_LEN];
    int  socials_count;
    char messengers[ARR_LEN][STR_LEN];
    int  messengers_count;
} Contact;

int contacts_count(void); // возвращает количество записей в телефонную книгу
Contact* contacts_array(void); // возвращает телефонную книгу
Contact* find_by_id(int id); // поиск по id

/* Добавление: Fam, Im обязательны; далее пары "ключ","значение" завершение NULL.
   Возврат: id (>0) или -1 при ошибке. */
int add_contact(const char* surname, const char* name, ...);

/* Редактирование: id, пары "ключ","значение", NULL.
   Возврат: true — успех, false — не найдено или провал редактирования. */
bool edit_contact(int id, ...);

/* Удаление по id. Возврат: true — успех, false — не найдено. */
bool remove_contact(int id);

#endif