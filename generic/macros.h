#pragma once

// This file only defines macros without expanding them.  Therefore it
// should never include any headers.  Clients are expected to include
// the necessary headers.

// needs GCC
#define likely(x) __builtin_expect(x, 1)
#define unlikely(x) __builtin_expect(x, 0)

// needs <stdlib.h>
#define with_file(f, path, mode)                                               \
    for (FILE *f = fopen(path, mode), *_continue = (FILE *)(size_t)1;          \
         f != NULL && ((size_t)_continue || fclose(f));                        \
         _continue = (FILE *)(size_t)0)

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
