CC     = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra
OBJS   = main.o gui.o conveyor.o
TARGET = conveyor

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

main.o:     main.c conveyor.h gui.h
gui.o:      gui.c gui.h conveyor.h
conveyor.o: conveyor.c conveyor.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: run clean

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)