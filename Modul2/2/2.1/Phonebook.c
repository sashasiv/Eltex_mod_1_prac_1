/* phonebook.c — работа с данными */
#include "phonebook.h"
#include <stdarg.h>

static Contact contacts[MAX_CONTACTS];
static int     g_count = 0;
static int     next_id = 1;

/* ---- внутренние (static) ---- */

static void _copy_str(char* dst, const char* src)
{
    if (!src) { dst[0] = '\0'; return; }
    strncpy(dst, src, STR_LEN - 1);
    dst[STR_LEN - 1] = '\0'; // на случай ввода > STR_LEN
}

static bool _is_blank(const char* s)
{
    if (!s) return true;
    for (int i = 0; s[i]; ++i) {
        unsigned char ch = (unsigned char)s[i];
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r' && ch != '\v' && ch != '\f')
            return false;
    }
    return true;
}

static bool _add_to_list(char list[][STR_LEN], int* count, const char* value)
{
    if (_is_blank(value)) return false;
    if (*count >= ARR_LEN)  return false;
    _copy_str(list[*count], value);
    (*count)++;
    return true;
}

/* удалить элемент по индексу 0..count-1; сдвиг хвоста влево */
static int _remove_from_list(char list[][STR_LEN], int* count, int index)
{
    if (index < 0 || index >= *count) return 0;
    for (int j = index; j < *count - 1; ++j)
        _copy_str(list[j], list[j + 1]);
    (*count)--;
    return 1;
}

static int _find_index_by_id(int id)
{
    for (int i = 0; i < g_count; ++i)
        if (contacts[i].id == id) return i;
    return -1;
}

static int _parse_index(const char* value)
{
    if (!value || value[0] < '1' || value[0] > '9') return 0;
    return value[0] - '0';
}

/* правка полей конкретной "ячейки" — не трогает массив */
static bool _set_field(Contact* c, const char* field, const char* value)
{
    /* ---- скаляры ---- */
    if (strcmp(field, "Fam") == 0) {
        if (_is_blank(value)) return false;
        _copy_str(c->Fam, value);
    }
    else if (strcmp(field, "Im") == 0) {
        if (_is_blank(value)) return false;
        _copy_str(c->Im, value);
    }
    else if (strcmp(field, "Otch") == 0) {
        if (_is_blank(value)) c->Otch[0] = '\0';
        else                  _copy_str(c->Otch, value);
    }
    else if (strcmp(field, "workplace") == 0) {
        if (_is_blank(value)) c->workplace[0] = '\0';
        else                  _copy_str(c->workplace, value);
    }
    else if (strcmp(field, "position") == 0) {
        if (_is_blank(value)) c->position[0] = '\0';
        else                  _copy_str(c->position, value);
    }
    /* ---- списки: add  ---- */
    else if (strcmp(field, "phone_add") == 0) {
        if (!_add_to_list(c->phones, &c->phones_count, value)) return false;
    }
    else if (strcmp(field, "email_add") == 0) {
        if (!_add_to_list(c->emails, &c->emails_count, value)) return false;
    }
    else if (strcmp(field, "social_add") == 0) {
        if (!_add_to_list(c->socials, &c->socials_count, value)) return false;
    }
    else if (strcmp(field, "messenger_add") == 0) {
        if (!_add_to_list(c->messengers, &c->messengers_count, value)) return false;
    }

    /* ---- списки: remove  ---- */
    else if (strcmp(field, "phone_remove") == 0) {
        if (!_remove_from_list(c->phones, &c->phones_count, _parse_index(value) - 1)) return false;
    }
    else if (strcmp(field, "email_remove") == 0) {
        if (!_remove_from_list(c->emails, &c->emails_count, _parse_index(value) - 1)) return false;
    }
    else if (strcmp(field, "social_remove") == 0) {
        if (!_remove_from_list(c->socials, &c->socials_count, _parse_index(value) - 1)) return false;
    }
    else if (strcmp(field, "messenger_remove") == 0) {
        if (!_remove_from_list(c->messengers, &c->messengers_count, _parse_index(value) - 1)) return false;
    }

    else return false;
    return true;
}



/* ---- внешние ---- */

int contacts_count(void) { return g_count; }

Contact* contacts_array(void) { return contacts; }

Contact* find_by_id(int id)
{
    int idx = _find_index_by_id(id);
    return idx < 0 ? NULL : &contacts[idx];
}

int add_contact(const char* fam, const char* im, ...)
{
    if (g_count >= MAX_CONTACTS) return -1;
    if (!fam || !*fam || !im || !*im) return -1;

    Contact* c = &contacts[g_count];
    memset(c, 0, sizeof(*c));
    c->id = next_id++;
    _copy_str(c->Fam, fam);
    _copy_str(c->Im, im);

    va_list ap;
    va_start(ap, im);
    const char* key;
    while ((key = va_arg(ap, const char*)) != NULL) {
        const char* val = va_arg(ap, const char*);
        if (!val) break;
        _set_field(c, key, val);
    }
    va_end(ap);

    g_count++;
    return c->id;
}

bool edit_contact(int id, ...)
{
    int idx = _find_index_by_id(id);
    if (idx < 0) return false;

    Contact* c = &contacts[idx];       

    va_list ap;
    va_start(ap, id);
    const char* key;
    while ((key = va_arg(ap, const char*)) != NULL) {
        const char* val = va_arg(ap, const char*);
        if (!val) break;                   /* ключ без значения — некорректный вызов */
        if(!_set_field(c, key, val)) return false;
    }
    va_end(ap);

    return true;
}

bool remove_contact(int id)
{
    int idx = _find_index_by_id(id);
    if (idx < 0) return false;
    for (int i = idx; i < g_count - 1; ++i)
        contacts[i] = contacts[i + 1];
    g_count--;
    return true;
}