#ifndef CONFIG_H
#define CONFIG_H

 /* --- размеры и приоритеты очередей --- */
#define MSG_MAX           512    /* макс. длина текста сообщения, байт */
#define PRIO_CONNECT       30    /* ВЫШЕ приоритет сообщения о подключении */
#define PRIO_NORMAL        20    /* приоритет обычных сообщений          */
#define PRIO_STOP          10    /* НИЖЕ обычного -> стоп идёт в конец    */

/* --- тайминги --- */
#define TICK_MS           100    /* шаг mq_timedreceive/mq_timedsend, мс  */
#define GRACE_TIMEOUT_MS 5000    /* ожидание ответного стопа, мс          */

/* --- параметры очередей --- */
#define Q_MAXMSG           10    /* макс. сообщений в очереди            */
#define Q_MODE           0600    /* права на очереди                     */
#define NAME_MAX_LEN      256    /* буфер под имя очереди                */

#endif /* CONFIG_H */