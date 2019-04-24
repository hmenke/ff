#include "scandir.h"
#include "macros.h"

// C standard library
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// POSIX C library
#include <dirent.h>

int ff_scandir(const char *dir, struct dirent ***namelist,
               int (*select)(const struct dirent *, const void *const data),
               const void *const data) {
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        return -1;
    }

    int save = errno;
    errno = 0;

    int result = -1;

    size_t vsize = 16;
    struct dirent **v = (struct dirent **)malloc(vsize * sizeof(*v));
    if (v == NULL) {
        goto fail;
    }
    size_t cnt = 0;
    struct dirent *d;
    while ((d = readdir(dp)) != NULL) {
        if (select != NULL) {
            int selected = (*select)(d, data);
            errno = 0;
            if (!selected) {
                continue;
            }
        }

        if (unlikely(cnt == vsize)) {
            vsize *= 2;
            struct dirent **vnew =
                (struct dirent **)realloc(v, vsize * sizeof(*v));
            if (vnew == NULL) {
                break;
            }
            v = vnew;
        }

        size_t dsize = &d->d_name[_D_ALLOC_NAMLEN(d)] - (char *)d;
        struct dirent *vnew = (struct dirent *)malloc(dsize);
        if (vnew == NULL) {
            break;
        }
        v[cnt++] = (struct dirent *)memcpy(vnew, d, dsize);

        errno = 0;
    }

    if (likely(errno == 0)) {
        qsort(v, cnt, sizeof(*v),
              (int (*)(const void *, const void *))alphasort);
        *namelist = v;
        result = cnt;
    } else {
        for (size_t i = 0; i < cnt; ++i) {
            free(v[i]);
        }
        free(v);
        result = -1;
    }

fail:
    closedir(dp);
    if (result >= 0) {
        errno = save;
    }
    return result;
}
