CC      = gcc
AR      = ar
CFLAGS  = -Wall -Wextra -std=c11 -I.
PICFLAG = -fPIC

LIB_A   = liblist.a
LIB_SO  = liblist.so
OBJS    = list.o quartile.o

.PHONY: all clean test

all: $(LIB_A) $(LIB_SO)

# статическая (для test_list / сравнения)
$(LIB_A): $(OBJS)
	$(AR) rcs $@ $^

# динамическая (для app)
$(LIB_SO): $(OBJS)
	$(CC) -shared -o $@ $^

list.o: list.c list.h quartile.h contact.h
	$(CC) $(CFLAGS) $(PICFLAG) -c list.c -o $@

quartile.o: quartile.c quartile.h list.h contact.h
	$(CC) $(CFLAGS) $(PICFLAG) -c quartile.c -o $@

test_list: test_list.c $(LIB_SO)
	$(CC) $(CFLAGS) test_list.c -L. -llist -o $@

test: test_list
	LD_LIBRARY_PATH=. ./test_list
clean:
	rm -f $(OBJS) $(LIB_A) $(LIB_SO) test_list