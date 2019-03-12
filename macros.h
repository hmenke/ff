#pragma once

#define with_pthread_mutex(mutex)                                              \
    for (int _continue = (pthread_mutex_lock(mutex), 1); _continue;            \
         _continue = 0, pthread_mutex_unlock(mutex))

#define foreach_opendir(entry, path)                                           \
    for (struct dirent *_d = (struct dirent *)opendir(path), *entry = NULL;    \
         _d != NULL &&                                                         \
         ((entry = readdir((DIR *)_d)) != NULL || closedir((DIR *)_d));)

#define foreach_queue_get(msg, queue)                                          \
    for (message *msg = NULL; (msg = queue_get(opt->q)) != NULL;               \
         message_free(msg))
