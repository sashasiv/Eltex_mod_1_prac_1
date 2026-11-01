CC       ?= gcc
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic

TARGET   := ip_router
SRCS     := core.c random.c ui.c main.c
OBJS     := $(SRCS:.c=.o)
DEPS     := $(OBJS:.o=.d)
HDRS     := core.h ui.h random.h

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

run: all
	./$(TARGET) 192.168.1.1 255.255.255.0 1000

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)