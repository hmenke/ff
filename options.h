#pragma once

#include "flagman.h"
#include "message.h"

// C standard library
#include <stdbool.h>

// PCRE
#include <pcre.h>

typedef enum { NONE, GLOB, REGEX } match_mode;

typedef struct {
    queue *q;
    flagman *flagman_lock;

    // tagged union
    union {
        pcre *re;
        const char *pattern;
    } match;
    match_mode mode;

    // program parameters
    int optind;
    unsigned char only_type;
    bool skip_hidden;
    long max_depth;
    bool colorize;
    bool icase;
    long nthreads;
} options;

int parse_options(int argc, char *argv[], options *opt);
