CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS += -lpcre -lpthread -lgit2

all: CFLAGS += -std=gnu99 -O3 -flto
all: ff

debug: CFLAGS += -std=gnu99 -pg -g -fstack-protector -D_FORTIFY_SOURCE=2
debug: ff

cpp: CC = c++ -x c++
cpp: ff

ff: dircolors.c \
    ff.c        \
    flagman.c   \
    message.c   \
    options.c
