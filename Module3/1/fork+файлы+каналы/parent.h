// parent.h
#ifndef PARENT_H
#define PARENT_H

/* Скопировать один файл через канал.
   wfd — пишем ребёнку, rfd — читаем от ребёнка.
   Возвращает:
     0  — файл успешно передан и подтверждён (DONE);
    -1  — ошибка (файл не открылся, протокол сломался, peer умер). */
int parent_copy_file(int wfd, int rfd, const char* path);

/* Уведомить ребёнка о завершении работы.
   Возвращает 0 при успехе, -1 при ошибке отправки. */
int parent_send_end(int wfd);

#endif