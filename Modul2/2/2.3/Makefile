# ---- компилятор и флаги ----
CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -O2
LDLIBS  = -lm

# ---- имена ----
TARGET  = Calculator
OBJS    = calc_ops.o main.o
HDR     = calc.h

# ---- сборка ----
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

calc_ops.o: calc_ops.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

# ---- очистка ----
clean:
	rm -f $(OBJS) $(TARGET)

# ---- пересборка ----
rebuild: clean $(TARGET)

.PHONY: clean rebuild