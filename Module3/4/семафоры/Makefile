# ============================================================================
#  Проект «Производитель-Потребители»: System V и POSIX бэкенды.
#
#  Цели:
#    make            — собрать оба варианта (sv + px)
#    make sv         — только System V  (задание 4)
#    make px         — только POSIX     (задание 5)
#    make run-sv N=<N> — запустить SV-вариант с N потребителями (по умолчанию 2)
#    make run-px N=<N> — запустить POSIX-вариант с N потребителями
#    make clean      — удалить бинарники и объектные файлы
#    make clean-ipc  — удалить оставшиеся IPC-ресурсы (после сбоев)
# ============================================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -O2
LDFLAGS =

# Общие части
HEADERS = config.h shm_list.h
OPS     = shm_list_ops.c

# Исходники приложения
APP_SRC = producer.c consumer.c run.c

# --- System V ---
SV_BACKEND = shm_list_sv.c
SV_BINS    = producer_sv consumer_sv run_sv

# --- POSIX ---
PX_BACKEND = shm_list_px.c
PX_BINS    = producer_px consumer_px run_px
PX_LIBS    = -pthread -lrt

# ---------------------------------------------------------------------------

all: sv px

sv: $(SV_BINS)

px: $(PX_BINS)

# --- System V: сборка ---

producer_sv: producer.c $(OPS) $(SV_BACKEND) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ producer.c $(OPS) $(SV_BACKEND) $(LDFLAGS)

consumer_sv: consumer.c $(OPS) $(SV_BACKEND) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ consumer.c $(OPS) $(SV_BACKEND) $(LDFLAGS)

run_sv: run.c $(OPS) $(SV_BACKEND) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ run.c $(OPS) $(SV_BACKEND) $(LDFLAGS)

# --- POSIX: сборка ---

producer_px: producer.c $(OPS) $(PX_BACKEND) $(HEADERS)
	$(CC) $(CFLAGS) -pthread -o $@ producer.c $(OPS) $(PX_BACKEND) $(PX_LIBS)

consumer_px: consumer.c $(OPS) $(PX_BACKEND) $(HEADERS)
	$(CC) $(CFLAGS) -pthread -o $@ consumer.c $(OPS) $(PX_BACKEND) $(PX_LIBS)

run_px: run.c $(OPS) $(PX_BACKEND) $(HEADERS)
	$(CC) $(CFLAGS) -pthread -o $@ run.c $(OPS) $(PX_BACKEND) $(PX_LIBS)

# --- Запуск ---
# run_* ожидают рядом с собой бинарники с именами ./producer и ./consumer.
# Делаем симлинки перед запуском, чтобы run.c не знал про _sv/_px.

run-sv: sv
	@ln -sf producer_sv producer
	@ln -sf consumer_sv consumer
	./run_sv $(N)

run-px: px
	@ln -sf producer_px producer
	@ln -sf consumer_px consumer
	./run_px $(N)

# --- Уборка ---

clean:
	rm -f $(SV_BINS) $(PX_BINS) producer consumer *.o

clean-ipc:
	-ipcrm -M 0x1234 2>/dev/null || true
	-ipcrm -S 0x1235 2>/dev/null || true
	-rm -f /dev/shm/prod_cons_shm /dev/shm/sem.prod_cons_sem

.PHONY: all sv px run-sv run-px check clean clean-ipc