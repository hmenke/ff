CFLAGS += -Wall -Wextra -Wpedantic -std=gnu99 -O3
LDLIBS += -lpcre -lpthread -lrt

ff: ff.c dircolors.c message.c flagman.c
