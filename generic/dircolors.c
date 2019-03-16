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
    if (strlen(str) <= sizeof(unsigned long)) {
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
        return "";
    }

    switch (statbuf.st_mode & S_IFMT) {
    case S_IFBLK:
        return DIRCOLOR_BLK;
    case S_IFCHR:
        return DIRCOLOR_CHR;
    case S_IFDIR:
        switch (statbuf.st_mode & (S_ISVTX | S_IWOTH)) {
        case S_ISVTX | S_IWOTH:
            return DIRCOLOR_STICKY_OTHER_WRITABLE;
        case S_IWOTH:
            return DIRCOLOR_OTHER_WRITABLE;
        case S_ISVTX:
            return DIRCOLOR_STICKY;
        default:
            return DIRCOLOR_DIR;
        }
    case S_IFIFO:
        return DIRCOLOR_FIFO;
    case S_IFLNK:
        return DIRCOLOR_LINK;
    case S_IFSOCK:
        return DIRCOLOR_SOCK;
    case S_IFREG:
        if (statbuf.st_mode & S_ISUID) {
            return DIRCOLOR_SETUID;
        }
        if (statbuf.st_mode & S_ISGID) {
            return DIRCOLOR_SETGID;
        }
        if (statbuf.st_mode & S_IEXEC) {
            return DIRCOLOR_EXEC;
        }
        break;
    default:
        break;
    }

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
    case HASH('t','a','r'):         return DIRCOLOR_ARCHIVE;
    case HASH('t','g','z'):         return DIRCOLOR_ARCHIVE;
    case HASH('a','r','c'):         return DIRCOLOR_ARCHIVE;
    case HASH('a','r','j'):         return DIRCOLOR_ARCHIVE;
    case HASH('t','a','z'):         return DIRCOLOR_ARCHIVE;
    case HASH('l','h','a'):         return DIRCOLOR_ARCHIVE;
    case HASH('l','z','4'):         return DIRCOLOR_ARCHIVE;
    case HASH('l','z','h'):         return DIRCOLOR_ARCHIVE;
    case HASH('l','z','m','a'):     return DIRCOLOR_ARCHIVE;
    case HASH('t','l','z'):         return DIRCOLOR_ARCHIVE;
    case HASH('t','x','z'):         return DIRCOLOR_ARCHIVE;
    case HASH('t','z','o'):         return DIRCOLOR_ARCHIVE;
    case HASH('t','7','z'):         return DIRCOLOR_ARCHIVE;
    case HASH('z','i','p'):         return DIRCOLOR_ARCHIVE;
    case HASH('z'):                 return DIRCOLOR_ARCHIVE;
    case HASH('d','z'):             return DIRCOLOR_ARCHIVE;
    case HASH('g','z'):             return DIRCOLOR_ARCHIVE;
    case HASH('l','r','z'):         return DIRCOLOR_ARCHIVE;
    case HASH('l','z'):             return DIRCOLOR_ARCHIVE;
    case HASH('l','z','o'):         return DIRCOLOR_ARCHIVE;
    case HASH('x','z'):             return DIRCOLOR_ARCHIVE;
    case HASH('z','s','t'):         return DIRCOLOR_ARCHIVE;
    case HASH('t','z','s','t'):     return DIRCOLOR_ARCHIVE;
    case HASH('b','z','2'):         return DIRCOLOR_ARCHIVE;
    case HASH('b','z'):             return DIRCOLOR_ARCHIVE;
    case HASH('t','b','z'):         return DIRCOLOR_ARCHIVE;
    case HASH('t','b','z','2'):     return DIRCOLOR_ARCHIVE;
    case HASH('t','z'):             return DIRCOLOR_ARCHIVE;
    case HASH('d','e','b'):         return DIRCOLOR_ARCHIVE;
    case HASH('r','p','m'):         return DIRCOLOR_ARCHIVE;
    case HASH('j','a','r'):         return DIRCOLOR_ARCHIVE;
    case HASH('w','a','r'):         return DIRCOLOR_ARCHIVE;
    case HASH('e','a','r'):         return DIRCOLOR_ARCHIVE;
    case HASH('s','a','r'):         return DIRCOLOR_ARCHIVE;
    case HASH('r','a','r'):         return DIRCOLOR_ARCHIVE;
    case HASH('a','l','z'):         return DIRCOLOR_ARCHIVE;
    case HASH('a','c','e'):         return DIRCOLOR_ARCHIVE;
    case HASH('z','o','o'):         return DIRCOLOR_ARCHIVE;
    case HASH('c','p','i','o'):     return DIRCOLOR_ARCHIVE;
    case HASH('7','z'):             return DIRCOLOR_ARCHIVE;
    case HASH('r','z'):             return DIRCOLOR_ARCHIVE;
    case HASH('c','a','b'):         return DIRCOLOR_ARCHIVE;
    case HASH('w','i','m'):         return DIRCOLOR_ARCHIVE;
    case HASH('s','w','m'):         return DIRCOLOR_ARCHIVE;
    case HASH('d','w','m'):         return DIRCOLOR_ARCHIVE;
    case HASH('e','s','d'):         return DIRCOLOR_ARCHIVE;
    // image formats
    case HASH('j','p','g'):         return DIRCOLOR_IMAGE;
    case HASH('j','p','e','g'):     return DIRCOLOR_IMAGE;
    case HASH('m','j','p','g'):     return DIRCOLOR_IMAGE;
    case HASH('m','j','p','e','g'): return DIRCOLOR_IMAGE;
    case HASH('g','i','f'):         return DIRCOLOR_IMAGE;
    case HASH('b','m','p'):         return DIRCOLOR_IMAGE;
    case HASH('p','b','m'):         return DIRCOLOR_IMAGE;
    case HASH('p','g','m'):         return DIRCOLOR_IMAGE;
    case HASH('p','p','m'):         return DIRCOLOR_IMAGE;
    case HASH('t','g','a'):         return DIRCOLOR_IMAGE;
    case HASH('x','b','m'):         return DIRCOLOR_IMAGE;
    case HASH('x','p','m'):         return DIRCOLOR_IMAGE;
    case HASH('t','i','f'):         return DIRCOLOR_IMAGE;
    case HASH('t','i','f','f'):     return DIRCOLOR_IMAGE;
    case HASH('p','n','g'):         return DIRCOLOR_IMAGE;
    case HASH('s','v','g'):         return DIRCOLOR_IMAGE;
    case HASH('s','v','g','z'):     return DIRCOLOR_IMAGE;
    case HASH('m','n','g'):         return DIRCOLOR_IMAGE;
    case HASH('p','c','x'):         return DIRCOLOR_IMAGE;
    case HASH('m','o','v'):         return DIRCOLOR_IMAGE;
    case HASH('m','p','g'):         return DIRCOLOR_IMAGE;
    case HASH('m','p','e','g'):     return DIRCOLOR_IMAGE;
    case HASH('m','2','v'):         return DIRCOLOR_IMAGE;
    case HASH('m','k','v'):         return DIRCOLOR_IMAGE;
    case HASH('w','e','b','m'):     return DIRCOLOR_IMAGE;
    case HASH('o','g','m'):         return DIRCOLOR_IMAGE;
    case HASH('m','p','4'):         return DIRCOLOR_IMAGE;
    case HASH('m','4','v'):         return DIRCOLOR_IMAGE;
    case HASH('m','p','4','v'):     return DIRCOLOR_IMAGE;
    case HASH('v','o','b'):         return DIRCOLOR_IMAGE;
    case HASH('q','t'):             return DIRCOLOR_IMAGE;
    case HASH('n','u','v'):         return DIRCOLOR_IMAGE;
    case HASH('w','m','v'):         return DIRCOLOR_IMAGE;
    case HASH('a','s','f'):         return DIRCOLOR_IMAGE;
    case HASH('r','m'):             return DIRCOLOR_IMAGE;
    case HASH('r','m','v','b'):     return DIRCOLOR_IMAGE;
    case HASH('f','l','c'):         return DIRCOLOR_IMAGE;
    case HASH('a','v','i'):         return DIRCOLOR_IMAGE;
    case HASH('f','l','i'):         return DIRCOLOR_IMAGE;
    case HASH('f','l','v'):         return DIRCOLOR_IMAGE;
    case HASH('g','l'):             return DIRCOLOR_IMAGE;
    case HASH('d','l'):             return DIRCOLOR_IMAGE;
    case HASH('x','c','f'):         return DIRCOLOR_IMAGE;
    case HASH('x','w','d'):         return DIRCOLOR_IMAGE;
    case HASH('y','u','v'):         return DIRCOLOR_IMAGE;
    case HASH('c','g','m'):         return DIRCOLOR_IMAGE;
    case HASH('e','m','f'):         return DIRCOLOR_IMAGE;
    // https://wiki.xiph.org/MIME_Types_and_File_Extensions
    case HASH('o','g','v'):         return DIRCOLOR_IMAGE;
    case HASH('o','g','x'):         return DIRCOLOR_IMAGE;
    // audio formats
    case HASH('a','a','c'):         return DIRCOLOR_AUDIO;
    case HASH('a','u'):             return DIRCOLOR_AUDIO;
    case HASH('f','l','a','c'):     return DIRCOLOR_AUDIO;
    case HASH('m','4','a'):         return DIRCOLOR_AUDIO;
    case HASH('m','i','d'):         return DIRCOLOR_AUDIO;
    case HASH('m','i','d','i'):     return DIRCOLOR_AUDIO;
    case HASH('m','k','a'):         return DIRCOLOR_AUDIO;
    case HASH('m','p','3'):         return DIRCOLOR_AUDIO;
    case HASH('m','p','c'):         return DIRCOLOR_AUDIO;
    case HASH('o','g','g'):         return DIRCOLOR_AUDIO;
    case HASH('r','a'):             return DIRCOLOR_AUDIO;
    case HASH('w','a','v'):         return DIRCOLOR_AUDIO;
    // https://wiki.xiph.org/MIME_Types_and_File_Extensions
    case HASH('o','g','a'):         return DIRCOLOR_AUDIO;
    case HASH('o','p','u','s'):     return DIRCOLOR_AUDIO;
    case HASH('s','p','x'):         return DIRCOLOR_AUDIO;
    case HASH('x','s','p','f'):     return DIRCOLOR_AUDIO;

    // 0 = hash failure (extension too long)
    case 0: return "";
    default: return "";
    }
    // clang-format on
}
