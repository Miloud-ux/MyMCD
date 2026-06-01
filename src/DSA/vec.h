#pragma once
#include <stddef.h>
#include <stdlib.h>

/* Implementation of a generic vector in C
 *  inspired by Sean Barret
 */

typedef struct VecHdr {
        size_t len;
        size_t cap;
        char vec[];
} VecHdr;

void *vec__grow(const void *vec, size_t new_len, size_t elem_size);

// access the starting addr of the struct with pointer arithmetic :
#define vec__hdr(v) ((VecHdr *)((char *)(v) - offsetof(VecHdr, vec)))

#define vec_len(v) ((v) ? vec__hdr(v)->len : 0)
#define vec_cap(v) ((v) ? vec__hdr(v)->cap : 0)
#define vec_end(v) ((v) + vec_len(v))
#define vec_sizeof(v) ((v) ? vec_len(v) * sizeof(*(v)) : 0)

#define vec_free(v) ((v) ? (free(vec__hdr(v)), (v) = NULL) : 0)
#define vec_fit(v, n) ((n) <= vec_cap(v) ? 0 : ((v) = vec__grow((v), (n), sizeof(*(v)))))

#define vec_push(v, ...) (vec_fit((v), 1 + vec_len(v)), (v)[vec__hdr(v)->len++] = (__VA_ARGS__))
#define vec_pop(v) ((v)[--vec__hdr(v)->len])
#define vec_clear(v) ((v) ? vec__hdr(v)->len = 0 : 0)
