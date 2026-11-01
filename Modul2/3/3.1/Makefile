# ---- компилятор и флаги ----
CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L

# ---- имена ----
TARGET  := permcalc
SRCS    := core.c ui.c main.c
OBJS    := $(SRCS:.c=.o)
DEPS    := $(OBJS:.o=.d)
HDRS    := core.h ui.h

# ---------- Цели ----------

.PHONY: all clean debug release run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

# Отладочная сборка
debug: CFLAGS += -g -O0 -DDEBUG
debug: clean all

# Релизная сборка
release: CFLAGS += -O2 -DNDEBUG
release: clean all

# Запуск
run: all
	./$(TARGET)

# Очистка
clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)