#pragma once

typedef struct _flagman flagman;

flagman *flagman_new();

void flagman_free(flagman *flagman_lock);

void flagman_acquire(flagman *flagman_lock);

void flagman_release(flagman *flagman_lock);

void flagman_wait(flagman *flagman_lock);
