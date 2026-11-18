CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2 -g
CPPFLAGS = -D_POSIX_C_SOURCE=200809L

SRC = main.c common.c broker.c publisher.c subscriber.c
OBJ = $(SRC:.c=.o)
BIN = pubsub

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# зависимости от заголовков (простая версия)
main.o:       main.c broker.h publisher.h subscriber.h common.h config.h
common.o:     common.c common.h config.h
broker.o:     broker.c broker.h common.h config.h
publisher.o:  publisher.c publisher.h common.h config.h
subscriber.o: subscriber.c subscriber.h common.h config.h

clean:
	rm -f $(OBJ) $(BIN)