CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -pthread
LDFLAGS = -pthread

TARGET  = chat
OBJS    = main.o net.o protocol.o receiver.o debug.o
HDRS    = chat.h net.h protocol.h receiver.h debug.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# Тесты парсера — отдельный бинарник, только protocol + test
test_proto: test_proto.o protocol.o
	$(CC) $(LDFLAGS) -o $@ $^

# Прогон тестов
check: test_proto
	./test_proto

clean:
	rm -f $(OBJS) $(TARGET) test_proto test_proto.o

.PHONY: all clean test_proto check