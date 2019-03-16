#include "flagman.h"
#include "macros.h"

// C standard library
#include <stdlib.h>

// POSIX C library
#include <pthread.h>

// Very nice idea for a counting lock
// https://github.com/pelotoncycle/directory

// Named after the traffic control workers who manage two way
// traffic on blind single way roads.  They count the cars going in
// and blocking opposing traffic until the same number have exited.

struct _flagman {
    pthread_mutex_t completion_lock;
    pthread_mutex_t count_lock;
    int count;
};

flagman *flagman_new() {
    flagman *flagman_lock = (flagman *)malloc(sizeof(flagman));
    pthread_mutex_init(&flagman_lock->completion_lock, NULL);
    pthread_mutex_init(&flagman_lock->count_lock, NULL);
    flagman_lock->count = 0;
    return flagman_lock;
}

void flagman_free(flagman *flagman_lock) {
    if (flagman_lock == NULL) {
        return;
    }
    pthread_mutex_destroy(&flagman_lock->completion_lock);
    pthread_mutex_destroy(&flagman_lock->count_lock);
    free(flagman_lock);
}

void flagman_acquire(flagman *flagman_lock) {
    with_pthread_mutex(&flagman_lock->count_lock) {
        if (flagman_lock->count == 0) {
            pthread_mutex_lock(&flagman_lock->completion_lock);
        }
        flagman_lock->count += 1;
    }
}

void flagman_release(flagman *flagman_lock) {
    with_pthread_mutex(&flagman_lock->count_lock) {
        if (flagman_lock->count == 0) {
            break;
        }
        flagman_lock->count -= 1;
        if (flagman_lock->count == 0) {
            pthread_mutex_unlock(&flagman_lock->completion_lock);
        }
    }
}

void flagman_wait(flagman *flagman_lock) {
    pthread_mutex_lock(&flagman_lock->completion_lock);
    pthread_mutex_unlock(&flagman_lock->completion_lock);
}
