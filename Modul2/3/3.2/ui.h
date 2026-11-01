#ifndef UI_H
#define UI_H

#include <stdbool.h>

/* Запуск программы: разбор аргументов, имитация, вывод статистики.
   Возвращает 0 при успехе, 1 при ошибке. */
bool ui_run(int argc, char** argv);

#endif /* UI_H */