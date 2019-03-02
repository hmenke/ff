#include "dircolors.h"

// C standard library
#include <assert.h>
#include <stdio.h>
#include <string.h>

// POSIX C library
#include <sys/stat.h>


// Simple hash function for short strings

#define HASH_1(ARG1) ((0UL << 8) + ARG1)
#define HASH_2(ARG1, ARG2) ((HASH_1(ARG1) << 8) + ARG2)
#define HASH_3(ARG1, ARG2, ARG3) ((HASH_2(ARG1, ARG2) << 8) + ARG3)
#define HASH_4(ARG1, ARG2, ARG3, ARG4) ((HASH_3(ARG1, ARG2, ARG3) << 8) + ARG4)
#define HASH_5(ARG1, ARG2, ARG3, ARG4, ARG5)                                   \
    ((HASH_4(ARG1, ARG2, ARG3, ARG4) << 8) + ARG5)
#define HASH_6(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)                             \
    ((HASH_5(ARG1, ARG2, ARG3, ARG4, ARG5) << 8) + ARG6)
#define HASH_7(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)                       \
    ((HASH_6(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) << 8) + ARG6)
#define HASH_8(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)                 \
    ((HASH_7(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) << 8) + ARG8)

#define HASH_COUNT(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, func, ...)  \
    func

#define HASH(...)                                                              \
    HASH_COUNT(__VA_ARGS__, HASH_8(__VA_ARGS__), HASH_7(__VA_ARGS__),          \
               HASH_6(__VA_ARGS__), HASH_5(__VA_ARGS__), HASH_4(__VA_ARGS__),  \
               HASH_3(__VA_ARGS__), HASH_2(__VA_ARGS__), HASH_1(__VA_ARGS__),  \
               sentinel)

unsigned long hash(const char *str) {
    if(strlen(str) <= sizeof(unsigned long)) {
        return 0UL;
    }
    unsigned long hash = 0UL;
    char c;
    while ((c = *str++) != '\0') {
        hash <<= 8;
        hash += c;
    }
    return hash;
}

/*
const char *unhash(unsigned long hash) {
    char str[sizeof(unsigned long)];
    int i;
    for (i = sizeof(unsigned long) - 1; i >= 0; --i) {
        char c = hash & 0xFF;
        hash >>= 8;
        if (c == '\0')
            break;
        str[i] = c;
    }
    return strndup(str + i + 1, sizeof(unsigned long) - 1 + i);
}
*/

// The colors are taken from the GNU coreutils file dircolors.hin
//
// Copyright (C) 1996-2019 Free Software Foundation, Inc.
// Copying and distribution of this file, with or without modification,
// are permitted provided the copyright notice and this notice are preserved.
const char *dircolor(const char *realpath) {
    struct stat statbuf;
    if (lstat(realpath, &statbuf) != 0) {
        perror("lstat failed");
    }

    // clang-format off
    switch (statbuf.st_mode & S_IFMT) {
    case S_IFBLK:  return "\33[40;33;01m";
    case S_IFCHR:  return "\33[40;33;01m";
    case S_IFDIR:  return "\33[01;34m";
    case S_IFIFO:  return "\33[40;33m";
    case S_IFLNK:  return "\33[01;36m";
    case S_IFSOCK: return "\33[01;35m";
    case S_IFREG:  break;
    default: break;
    }

    if (statbuf.st_mode & S_IEXEC) { return "\33[01;32m"; }
    if (statbuf.st_mode & S_ISUID) { return "\33[37;41m"; }
    if (statbuf.st_mode & S_ISGID) { return "\33[30;43m"; }
    if (statbuf.st_mode & S_ISVTX) { return "\33[37;44m"; }
    // clang-format on

    // Extract the extension
    const char *basename = strrchr(realpath, '/');
    if (basename == NULL) {
        basename = realpath;
    }
    const char *ext = strrchr(basename, '.');
    ext = ext ? ext + 1 : "";

    // clang-format off
    switch (hash(ext)) {
    // archives or compressed (bright red)
    case HASH('t','a','r'):         return "\33[01;31m";
    case HASH('t','g','z'):         return "\33[01;31m";
    case HASH('a','r','c'):         return "\33[01;31m";
    case HASH('a','r','j'):         return "\33[01;31m";
    case HASH('t','a','z'):         return "\33[01;31m";
    case HASH('l','h','a'):         return "\33[01;31m";
    case HASH('l','z','4'):         return "\33[01;31m";
    case HASH('l','z','h'):         return "\33[01;31m";
    case HASH('l','z','m','a'):     return "\33[01;31m";
    case HASH('t','l','z'):         return "\33[01;31m";
    case HASH('t','x','z'):         return "\33[01;31m";
    case HASH('t','z','o'):         return "\33[01;31m";
    case HASH('t','7','z'):         return "\33[01;31m";
    case HASH('z','i','p'):         return "\33[01;31m";
    case HASH('z'):                 return "\33[01;31m";
    case HASH('d','z'):             return "\33[01;31m";
    case HASH('g','z'):             return "\33[01;31m";
    case HASH('l','r','z'):         return "\33[01;31m";
    case HASH('l','z'):             return "\33[01;31m";
    case HASH('l','z','o'):         return "\33[01;31m";
    case HASH('x','z'):             return "\33[01;31m";
    case HASH('z','s','t'):         return "\33[01;31m";
    case HASH('t','z','s','t'):     return "\33[01;31m";
    case HASH('b','z','2'):         return "\33[01;31m";
    case HASH('b','z'):             return "\33[01;31m";
    case HASH('t','b','z'):         return "\33[01;31m";
    case HASH('t','b','z','2'):     return "\33[01;31m";
    case HASH('t','z'):             return "\33[01;31m";
    case HASH('d','e','b'):         return "\33[01;31m";
    case HASH('r','p','m'):         return "\33[01;31m";
    case HASH('j','a','r'):         return "\33[01;31m";
    case HASH('w','a','r'):         return "\33[01;31m";
    case HASH('e','a','r'):         return "\33[01;31m";
    case HASH('s','a','r'):         return "\33[01;31m";
    case HASH('r','a','r'):         return "\33[01;31m";
    case HASH('a','l','z'):         return "\33[01;31m";
    case HASH('a','c','e'):         return "\33[01;31m";
    case HASH('z','o','o'):         return "\33[01;31m";
    case HASH('c','p','i','o'):     return "\33[01;31m";
    case HASH('7','z'):             return "\33[01;31m";
    case HASH('r','z'):             return "\33[01;31m";
    case HASH('c','a','b'):         return "\33[01;31m";
    case HASH('w','i','m'):         return "\33[01;31m";
    case HASH('s','w','m'):         return "\33[01;31m";
    case HASH('d','w','m'):         return "\33[01;31m";
    case HASH('e','s','d'):         return "\33[01;31m";
    // image formats
    case HASH('j','p','g'):         return "\33[01;35m";
    case HASH('j','p','e','g'):     return "\33[01;35m";
    case HASH('m','j','p','g'):     return "\33[01;35m";
    case HASH('m','j','p','e','g'): return "\33[01;35m";
    case HASH('g','i','f'):         return "\33[01;35m";
    case HASH('b','m','p'):         return "\33[01;35m";
    case HASH('p','b','m'):         return "\33[01;35m";
    case HASH('p','g','m'):         return "\33[01;35m";
    case HASH('p','p','m'):         return "\33[01;35m";
    case HASH('t','g','a'):         return "\33[01;35m";
    case HASH('x','b','m'):         return "\33[01;35m";
    case HASH('x','p','m'):         return "\33[01;35m";
    case HASH('t','i','f'):         return "\33[01;35m";
    case HASH('t','i','f','f'):     return "\33[01;35m";
    case HASH('p','n','g'):         return "\33[01;35m";
    case HASH('s','v','g'):         return "\33[01;35m";
    case HASH('s','v','g','z'):     return "\33[01;35m";
    case HASH('m','n','g'):         return "\33[01;35m";
    case HASH('p','c','x'):         return "\33[01;35m";
    case HASH('m','o','v'):         return "\33[01;35m";
    case HASH('m','p','g'):         return "\33[01;35m";
    case HASH('m','p','e','g'):     return "\33[01;35m";
    case HASH('m','2','v'):         return "\33[01;35m";
    case HASH('m','k','v'):         return "\33[01;35m";
    case HASH('w','e','b','m'):     return "\33[01;35m";
    case HASH('o','g','m'):         return "\33[01;35m";
    case HASH('m','p','4'):         return "\33[01;35m";
    case HASH('m','4','v'):         return "\33[01;35m";
    case HASH('m','p','4','v'):     return "\33[01;35m";
    case HASH('v','o','b'):         return "\33[01;35m";
    case HASH('q','t'):             return "\33[01;35m";
    case HASH('n','u','v'):         return "\33[01;35m";
    case HASH('w','m','v'):         return "\33[01;35m";
    case HASH('a','s','f'):         return "\33[01;35m";
    case HASH('r','m'):             return "\33[01;35m";
    case HASH('r','m','v','b'):     return "\33[01;35m";
    case HASH('f','l','c'):         return "\33[01;35m";
    case HASH('a','v','i'):         return "\33[01;35m";
    case HASH('f','l','i'):         return "\33[01;35m";
    case HASH('f','l','v'):         return "\33[01;35m";
    case HASH('g','l'):             return "\33[01;35m";
    case HASH('d','l'):             return "\33[01;35m";
    case HASH('x','c','f'):         return "\33[01;35m";
    case HASH('x','w','d'):         return "\33[01;35m";
    case HASH('y','u','v'):         return "\33[01;35m";
    case HASH('c','g','m'):         return "\33[01;35m";
    case HASH('e','m','f'):         return "\33[01;35m";
    // https://wiki.xiph.org/MIME_Types_and_File_Extensions
    case HASH('o','g','v'):         return "\33[01;35m";
    case HASH('o','g','x'):         return "\33[01;35m";
    // audio formats
    case HASH('a','a','c'):         return "\33[00;36m";
    case HASH('a','u'):             return "\33[00;36m";
    case HASH('f','l','a','c'):     return "\33[00;36m";
    case HASH('m','4','a'):         return "\33[00;36m";
    case HASH('m','i','d'):         return "\33[00;36m";
    case HASH('m','i','d','i'):     return "\33[00;36m";
    case HASH('m','k','a'):         return "\33[00;36m";
    case HASH('m','p','3'):         return "\33[00;36m";
    case HASH('m','p','c'):         return "\33[00;36m";
    case HASH('o','g','g'):         return "\33[00;36m";
    case HASH('r','a'):             return "\33[00;36m";
    case HASH('w','a','v'):         return "\33[00;36m";
    // https://wiki.xiph.org/MIME_Types_and_File_Extensions
    case HASH('o','g','a'):         return "\33[00;36m";
    case HASH('o','p','u','s'):     return "\33[00;36m";
    case HASH('s','p','x'):         return "\33[00;36m";
    case HASH('x','s','p','f'):     return "\33[00;36m";

    // 0 = hash failure (extension too long)
    case 0: return "";
    default: return "";
    }
    // clang-format on
}
