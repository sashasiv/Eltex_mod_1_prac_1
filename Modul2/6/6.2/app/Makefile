CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11 -I../liblist

APP_OBJS = ui.o phonebook.o
LIB      = ../liblist/liblist.so
TARGET   = phonebook

# rpath: искать .so относительно app (../liblist)
RPATH    = -Wl,-rpath,'$$ORIGIN/../liblist'

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(APP_OBJS)
	$(CC) $(APP_OBJS) ../liblist/liblist.so $(RPATH) -o $(TARGET)
	
ui.o: ui.c phonebook.h ../liblist/list.h ../liblist/contact.h
	$(CC) $(CFLAGS) -c ui.c -o $@

phonebook.o: phonebook.c phonebook.h ../liblist/list.h ../liblist/contact.h
	$(CC) $(CFLAGS) -c phonebook.c -o $@

clean:
	rm -f $(APP_OBJS) $(TARGET)