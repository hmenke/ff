CFLAGS += -Wall -Wextra -Wpedantic -Igeneric -isystemgit
LDLIBS += -lz -lpthread

pcre: LDLIBS += -lpcre
pcre: release

posix: CFLAGS += -DUSE_POSIX_REGEX
posix: release

release: CFLAGS += -std=gnu99 -O3 -flto
release: ff

debug: CFLAGS += -std=gnu99 -g -fstack-protector -D_FORTIFY_SOURCE=2
debug: LDLIBS += -lpcre
debug: ff

cpp: CC = c++ -x c++
cpp: ff

ff: generic/dircolors.c \
    generic/flagman.c   \
    generic/gitignore.c \
    generic/message.c   \
    ff.c                \
    options.c           \
    regex.c             \
    git/libgit.a        \
    git/xdiff/lib.a

git/libgit.a:
	$(MAKE) -C git libgit.a

git/xdiff/lib.a:
	$(MAKE) -C git xdiff/lib.a
