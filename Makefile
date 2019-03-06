CFLAGS += -Wall -Wextra -Wpedantic -std=gnu99 -O3 -flto
LDLIBS += -lpcre -lpthread -lrt

ff: ff.c dircolors.c message.c flagman.c

testc++: CC = c++ -x c++
testc++: ff
