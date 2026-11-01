#ifndef UI_H
#define UI_H

/* Пункт 1: запросить у пользователя права (octal или symbolic),
   распарсить, вывести буквенное/цифровое/битовое представление.
   При успехе обновляет *current_mode и *have_mode.
   Возвращает 0 при успехе, -1 при ошибке ввода/разбора. */
int ui_prompt_mode(unsigned* current_mode, int* have_mode);

/* Пункт 2: запросить имя файла, прочитать права через stat(),
   вывести три представления + сравнение с ls -l.
   При успехе обновляет *current_mode и *have_mode.
   Возвращает 0 при успехе, -1 при ошибке. */
int ui_prompt_file(unsigned* current_mode, int* have_mode);

int ui_prompt_chmod(unsigned* current_mode, int have_mode);

/* Вспомогательные: печать mode во всех трёх представлениях. */
void ui_print_mode(unsigned mode);



#endif /* UI_H */