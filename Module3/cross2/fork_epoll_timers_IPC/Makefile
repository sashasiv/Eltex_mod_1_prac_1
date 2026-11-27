CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11 -D_GNU_SOURCE

TARGETS = taxi driver

all: $(TARGETS)

taxi: taxi.c taxi.h
	$(CC) $(CFLAGS) -o $@ taxi.c

driver: driver.c taxi.h
	$(CC) $(CFLAGS) -o $@ driver.c

clean:
	rm -f $(TARGETS)

.PHONY: all clean