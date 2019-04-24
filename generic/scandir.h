#pragma once

// POSIX C library
#include <dirent.h>

int ff_scandir(const char *dir, struct dirent ***namelist,
               int (*select)(const struct dirent *, const void *const data),
               const void *const data);

