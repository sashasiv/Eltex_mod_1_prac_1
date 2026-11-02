CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11

OBJS    = tree.o phonebook.o ui.o
TARGET  = phonebook

ifeq ($(OS),Windows_NT)
    RM     = del /Q
    RM_ERR = 2>NUL
    EXE    = .exe
else
    RM     = rm -f
    RM_ERR =
    EXE    =
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

tree.o: tree.c tree.h contact.h
	$(CC) $(CFLAGS) -c tree.c -o $@

phonebook.o: phonebook.c phonebook.h tree.h contact.h
	$(CC) $(CFLAGS) -c phonebook.c -o $@

ui.o: ui.c phonebook.h tree.h contact.h
	$(CC) $(CFLAGS) -c ui.c -o $@

clean:
	-$(RM) $(OBJS) $(TARGET)$(EXE) $(RM_ERR)