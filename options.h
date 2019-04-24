#pragma once

#include "flagman.h"
#include "message.h"
#include "regex.h"

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
        regex *re;
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
    bool no_ignore;
    long nthreads;
    const char *ext;
} options;

enum {
    OPTIONS_SUCCESS = 0,
    OPTIONS_FAILURE = 1,
    OPTIONS_HELP = 2,
};

int parse_options(int argc, char *argv[], options *opt);
