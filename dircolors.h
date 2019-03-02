#pragma once

// These colors are taken from the GNU coreutils file dircolors.hin
//
// Copyright (C) 1996-2019 Free Software Foundation, Inc.
// Copying and distribution of this file, with or without modification,
// are permitted provided the copyright notice and this notice are preserved.
#define DIRCOLOR_RESET "\33[0m"

#define DIRCOLOR_BLK "\33[40;33;01m"
#define DIRCOLOR_CHR "\33[40;33;01m"
#define DIRCOLOR_DIR "\33[01;34m"
#define DIRCOLOR_FIFO "\33[40;33m"
#define DIRCOLOR_LINK "\33[01;36m"
#define DIRCOLOR_SOCK "\33[01;35m"

#define DIRCOLOR_ORPHAN "\33[40;31;01"
#define DIRCOLOR_CAPABILITY "\33[30;41m"

#define DIRCOLOR_SETUID "\33[37;41m"
#define DIRCOLOR_SETGID "\33[30;43m"
#define DIRCOLOR_EXEC "\33[01;32m"

#define DIRCOLOR_STICKY_OTHER_WRITABLE "\33[30;42m"
#define DIRCOLOR_OTHER_WRITABLE "\33[34;42m"
#define DIRCOLOR_STICKY "\33[37;44m"

#define DIRCOLOR_ARCHIVE "\33[01;31m"
#define DIRCOLOR_IMAGE "\33[01;35m"
#define DIRCOLOR_AUDIO "\33[00;36m"

const char *dircolor(const char *ext);
