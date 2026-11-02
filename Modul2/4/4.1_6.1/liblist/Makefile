CC      = gcc
AR      = ar
CFLAGS  = -Wall -Wextra -std=c11 -I.

LIB     = liblist.a
OBJS    = list.o quartile.o

.PHONY: all clean test

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

list.o: list.c list.h quartile.h contact.h
	$(CC) $(CFLAGS) -c list.c -o $@

quartile.o: quartile.c quartile.h list.h contact.h
	$(CC) $(CFLAGS) -c quartile.c -o $@

test_list: test_list.c $(LIB)
	$(CC) $(CFLAGS) test_list.c -L. -llist -o $@

test: test_list
	./test_list

clean:
	rm -f $(OBJS) $(LIB) test_list