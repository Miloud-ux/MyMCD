#include "vec.h"
#include <assert.h>

#define MAX(x, y) ((x) >= (y) ? (x) : (y))

void *vec__grow(const void *vec, size_t new_len, size_t elem_size) {
    size_t new_cap = MAX(16, MAX(1 + 2 * vec_cap(vec), new_len));
    assert(new_len <= new_cap);

    size_t new_size = offsetof(VecHdr, vec) + new_cap * elem_size;
    VecHdr *new_hdr;

    if (vec) {
        new_hdr = realloc(vec__hdr(vec), new_size);
    } else {
        new_hdr = malloc(new_size);
        if (!new_hdr)
            return NULL;
        new_hdr->len = 0;
    }

    new_hdr->cap = new_cap;
    return new_hdr->vec;
}
