CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -pthread
LDFLAGS = -pthread

# Общие объектные файлы для сервера и клиента
COMMON_OBJS = net.o protocol.o debug.o

# Исполняемые файлы
TARGETS = server client

all: $(TARGETS)

# Сборка TCP-сервера (использует poll)
server: server.o $(COMMON_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Сборка TCP-клиента (использует select)
client: client.o $(COMMON_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Сборка тестов парсера (из Задания 6)
test_proto: test_proto.o protocol.o
	$(CC) $(LDFLAGS) -o $@ $^

# Зависимость объектных файлов от всех заголовков
%.o: %.c chat.h net.h protocol.h debug.h
	$(CC) $(CFLAGS) -c $< -o $@

# Запуск тестов парсера
check: test_proto
	./test_proto

clean:
	rm -f *.o server client test_proto

.PHONY: all clean check