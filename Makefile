CFLAGS += -Wall -Wextra -Wpedantic -std=gnu99 -O3
LDLIBS += -lpcre -lpthread

ff: ff.c dircolors.c serialize.c
