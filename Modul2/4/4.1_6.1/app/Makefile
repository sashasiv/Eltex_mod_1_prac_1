CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11 -I../liblist

APP_OBJS = ui.o phonebook.o
LIB      = ../liblist/liblist.a
TARGET   = phonebook

# кроссплатформенный clean
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

$(TARGET): $(APP_OBJS) $(LIB)
	$(CC) $(APP_OBJS) $(LIB) -o $(TARGET)

ui.o: ui.c phonebook.h ../liblist/list.h ../liblist/contact.h
	$(CC) $(CFLAGS) -c ui.c -o $@

phonebook.o: phonebook.c phonebook.h ../liblist/list.h ../liblist/contact.h
	$(CC) $(CFLAGS) -c phonebook.c -o $@

clean:
	-$(RM) $(APP_OBJS) $(TARGET)$(EXE) $(RM_ERR)