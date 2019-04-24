#include "regex.h"

// C standard library
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef USE_POSIX_REGEX
// POSIX C library
#include <regex.h>
#else
// PCRE
#include <pcre.h>
#endif

struct _regex {
#ifdef USE_POSIX_REGEX
    regex_t re;
#else
    pcre *re;
#endif
};

#ifdef USE_POSIX_REGEX
typedef void _regex_storage;
#else
struct _regex_storage {
    pcre_extra *extra;
    pcre_jit_stack *jit_stack;
};
#endif

regex *regex_compile(const char *pattern, bool icase) {
    regex *re = (regex *)malloc(sizeof(regex));
#ifdef USE_POSIX_REGEX
    int flags = REG_EXTENDED;
    if (icase) {
        flags |= REG_ICASE;
    }

    char errbuf[256];
    int rc = regcomp(&re->re, pattern, flags);
    if (rc != 0) {
        regerror(rc, &re->re, errbuf, 256);
        fprintf(stderr, "Invalid regex: %s\n", errbuf);
        regex_free(re);
        exit(1);
    }
#else
    int flags = PCRE_UCP | PCRE_UTF8;
    if (icase) {
        flags |= PCRE_CASELESS;
    }

    const char *error;
    int erroffset;
    re->re = pcre_compile(pattern, flags, &error, &erroffset, NULL);
    if (re->re == NULL) {
        fprintf(stderr, "Invalid regex: %s at %d\n", error, erroffset);
        regex_free(re);
        exit(1);
    }
#endif
    return re;
}

bool regex_match(regex *re, regex_storage *mem, const char *str, int len) {
#ifdef USE_POSIX_REGEX
    (void)mem;
    (void)len;
    if (regexec(&re->re, str, 0, NULL, 0) == 0) {
        return true;
    }
#else
    int ovector[3];
    if (pcre_jit_exec(re->re, mem->extra, str, len, 0, 0, ovector, 3,
                      mem->jit_stack) > 0) {
        return true;
    }
#endif
    return false;
}

void regex_free(regex *re) {
    if (re == NULL) {
        return;
    }
#ifdef USE_POSIX_REGEX
#else
    pcre_free(re->re);
#endif
    free(re);
    re = NULL;
}

regex_storage *regex_storage_new(regex *re) {
#ifdef USE_POSIX_REGEX
    (void)re;
    return NULL;
#else
    regex_storage *mem = (regex_storage *)malloc(sizeof(regex_storage));
    const char *error;
    mem->extra = pcre_study(re->re, PCRE_STUDY_JIT_COMPILE, &error);
    assert(mem->extra != NULL);
    mem->jit_stack = pcre_jit_stack_alloc(32 * 1024, 512 * 1024);
    assert(mem->jit_stack != NULL);
    pcre_assign_jit_stack(mem->extra, NULL, mem->jit_stack);
    return mem;
#endif
}

void regex_storage_free(regex_storage *mem) {
#ifdef USE_POSIX_REGEX
    (void)mem;
    return;
#else
    if (mem == NULL) {
        return;
    }
    pcre_free_study(mem->extra);
    pcre_jit_stack_free(mem->jit_stack);
    free(mem);
    mem = NULL;
#endif
}
