#include "regex.h"

// C standard library
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

// PCRE
#include <pcre.h>

struct _regex {
    pcre *re;
};

struct _regex_storage {
    pcre_extra *extra;
    pcre_jit_stack *jit_stack;
};

regex *regex_compile(const char *pattern, bool icase) {
    regex *re = (regex *)malloc(sizeof(regex *));

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

    return re;
}

bool regex_match(regex *re, regex_storage *mem, const char *str, int len) {
    int ovector[3];
    if (pcre_jit_exec(re->re, mem->extra, str, len, 0, 0, ovector, 3,
                      mem->jit_stack) > 0) {
        return true;
    }
    return false;
}

void regex_free(regex *re) {
    pcre_free(re->re);
    free(re);
}

regex_storage *regex_storage_new(regex *re) {
    regex_storage *mem = (regex_storage *)malloc(sizeof(regex_storage *));
    const char *error;
    mem->extra = pcre_study(re->re, PCRE_STUDY_JIT_COMPILE, &error);
    assert(mem->extra != NULL);
    mem->jit_stack = pcre_jit_stack_alloc(32 * 1024, 512 * 1024);
    assert(mem->jit_stack != NULL);
    pcre_assign_jit_stack(mem->extra, NULL, mem->jit_stack);
    return mem;
}

void regex_storage_free(regex_storage *mem) {
    pcre_free_study(mem->extra);
    pcre_jit_stack_free(mem->jit_stack);
    free(mem);
}
