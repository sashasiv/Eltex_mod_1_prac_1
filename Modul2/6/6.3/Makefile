CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -fPIC
LDLIBS  = -ldl -lm

MAIN_SRC = main.c loader.c tokenize.c parse.c
MAIN_OBJ = $(MAIN_SRC:.c=.o)
MAIN_HDR = calc.h loader.h plugin_common.h

PLUGINS   = add sub mul div pow root log sin cos tg not_plugin
PLUGIN_SO = $(addprefix plugins/,$(addsuffix .so,$(PLUGINS)))

all: calc $(PLUGIN_SO)

calc: $(MAIN_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c $(MAIN_HDR)
	$(CC) $(CFLAGS) -c -o $@ $<

plugins/%.so: plugins/%.c plugin_common.h calc.h
	$(CC) $(CFLAGS) -shared -o $@ $< -lm

clean:
	rm -f calc $(MAIN_OBJ) $(PLUGIN_SO)

run: all
	./calc plugins

.PHONY: all clean run