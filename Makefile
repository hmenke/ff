CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS += -lpcre -lpthread -lrt

all: CFLAGS += -std=gnu99 -O3 -flto
all: ff

debug: CFLAGS += -std=gnu99 -g
debug: ff

cpp: CC = c++ -x c++
cpp: ff

ff: dircolors.c \
    ff.c        \
    flagman.c   \
    message.c   \
    options.c
