# ---------- инструменты ----------
CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2 -g -D_GNU_SOURCE
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS  ?=

# ---------- файлы ----------
BIN     := copyfiles
OBJS    := main.o parent.o child.o util.o msg.o
SRCS    := main.c parent.c child.c util.c msg.c
HDRS    := msg.h util.h parent.h child.h

# ---------- цели ----------
.PHONY: all clean test

all: $(BIN)

# Линковка
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Компиляция: каждый .o зависит от своего .c и от ВСЕХ заголовков
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# Прогон тестового скрипта
test: $(BIN)
	./test.sh

clean:
	rm -f $(OBJS) $(BIN)

# Не удалять промежуточные .o при наличии только .c
.SECONDARY: $(OBJS)