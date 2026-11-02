#ifndef CONTACT_H
#define CONTACT_H

#define STR_LEN 64
#define ARR_LEN 9

typedef struct Contact {
    char Fam[STR_LEN];          /* ключ 1 */
    char Im[STR_LEN];           /* ключ 2 */
    char Otch[STR_LEN];         /* ключ 3 */
    char workplace[STR_LEN];    /* ключ 4 */
    char position[STR_LEN];     /* ключ 5 */
    char phones[ARR_LEN][STR_LEN];      int phones_count;
    char emails[ARR_LEN][STR_LEN];      int emails_count;
    char socials[ARR_LEN][STR_LEN];     int socials_count;
    char messengers[ARR_LEN][STR_LEN];  int messengers_count;
} Contact;

#endif