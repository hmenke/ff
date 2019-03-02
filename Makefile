CFLAGS += -Wall -Wextra -Wpedantic -std=gnu99 -O3
LDLIBS += -lpcre

ff: ff.c dircolors.c
