# ---- компилятор и флаги ----
CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -O2

# ---- имена ----
TARGET  = phonebook
OBJS    = phonebook.o ui.o
HDR     = phonebook.h

# ---- сборка ----
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

phonebook.o: phonebook.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

ui.o: ui.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

# ---- очистка ----
clean:
	rm -f $(OBJS) $(TARGET)

# ---- пересборка ----
rebuild: clean $(TARGET)

.PHONY: clean rebuild