CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2 -D_GNU_SOURCE
LDFLAGS :=

COMMON_OBJS := common.o udp_raw.o signals.o

.PHONY: all clean test

all: echo_server echo_client

echo_server: echo_server.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

echo_client: echo_client.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o echo_server echo_client

# запуск требует CAP_NET_RAW:
#   sudo ./echo_server
#   sudo ./echo_client
