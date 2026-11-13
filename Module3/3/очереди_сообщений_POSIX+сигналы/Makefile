# ============================================================
# Makefile — сборка чата.
#   make        — собрать ./chat
#   make clean  — удалить объектники и бинарник
# ============================================================

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -pthread -D_POSIX_C_SOURCE=200809L
LDFLAGS = -pthread -lrt

SRC = main.c common.c threads.c creator.c client.c cleaner.c
OBJ = $(SRC:.c=.o)
BIN = chat

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean