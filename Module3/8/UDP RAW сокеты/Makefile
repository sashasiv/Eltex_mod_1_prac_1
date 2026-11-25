CC      = gcc
CFLAGS  = -Wall -Wextra -O2

# Объектные файлы только для сниффера
OBJS    = main.o net.o sniff_parse.o filter.o sniff_output.o
HDRS    = sniffer.h net.h sniff_parse.h filter.h sniff_output.h
TARGET  = sniffer

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean