#pragma once

// This file only defines macros without expanding them.  Therefore it
// should never include any headers.  Clients are expected to include
// the necessary headers.

// needs <pthread.h>
#define with_pthread_mutex(mutex)                                              \
    for (int _continue = (pthread_mutex_lock(mutex), 1); _continue;            \
         _continue = 0, pthread_mutex_unlock(mutex))

// needs "message.h"
#define foreach_queue_get(msg, queue)                                          \
    for (message *msg = NULL; (msg = queue_get(opt->q)) != NULL;               \
         message_free(msg))

// needs <dirent.h>
#define foreach_opendir(entry, path)                                           \
    for (struct dirent *_d = (struct dirent *)opendir(path), *entry = NULL;    \
         (DIR *)_d != NULL &&                                                  \
         ((entry = readdir((DIR *)_d)) != NULL || closedir((DIR *)_d));)

// needs <dirent.h> (and maybe <sys/types.h> for ssize_t)
#define foreach_scandir(...)                                                   \
    foreach_scandir_count(                                                     \
        __VA_ARGS__, foreach_scandir_with_filter(__VA_ARGS__),                 \
        foreach_scandir_without_filter(__VA_ARGS__), sentinel)

#define foreach_scandir_count(ARG1, ARG2, ARG3, func, ...) func

#define foreach_scandir_without_filter(entry, path)                            \
    foreach_scandir_with_filter(entry, path, NULL)

#define foreach_scandir_with_filter(entry, path, filter)                       \
    for (struct dirent **_namelist = NULL, *entry = NULL,                      \
                       *_n = (struct dirent *)(ssize_t)scandir(                \
                           path, &_namelist, filter, alphasort),               \
                       *_i = (struct dirent *)(ssize_t)0;                      \
         ((ssize_t)_n) != -1 && ((ssize_t)_i) < ((ssize_t)_n) &&               \
         (entry = _namelist[((ssize_t)_i)]);                                   \
         _i = (struct dirent *)((ssize_t)_i + 1))
