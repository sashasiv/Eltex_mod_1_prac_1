/* ============================================================
 * cleaner.h — ручной уборщик.
 *
 * Запуск: ./chat --clean <name>
 * Удаляет <name>_1 и <name>_2 (ENOENT игнорируется).
 * 
 * Возврат: 0 — ок, -1 — ошибка mq_unlink (кроме ENOENT).
 * ============================================================ */
#ifndef CLEANER_H
#define CLEANER_H

int clean_queues(const char* base);

#endif /* CLEANER_H */