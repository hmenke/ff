CFLAGS += -Wall -Wextra -Wpedantic -Igeneric
LDLIBS += -lpcre -lpthread -lgit2

all: CFLAGS += -std=gnu99 -O3 -flto
all: ff

debug: CFLAGS += -std=gnu99 -g -fstack-protector -D_FORTIFY_SOURCE=2
debug: ff

cpp: CC = c++ -x c++
cpp: ff

ff: generic/dircolors.c \
    ff.c                \
    generic/flagman.c   \
    generic/message.c   \
    options.c           \
    regex.c
