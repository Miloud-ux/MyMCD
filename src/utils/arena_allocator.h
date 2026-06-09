#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct ArenaBlock {
        uintptr_t base;
        size_t cap;
        size_t offset;
        struct ArenaBlock *next;
} ArenaBlock;

typedef struct Arena {
        ArenaBlock *head; // to free later
        ArenaBlock *current;
        size_t default_block_size;
} Arena;

void arena_init(Arena *a, size_t size);
ArenaBlock *arena_add_block(Arena *a, size_t size);
void *arena_alloc_aligned(Arena *a, size_t size, size_t align);

/* === Temp arena ===
 *  Temp arena is just an arena but we remember the
 *  offset of the current block so that when we are
 *  done working with this temporary memory we can
 *  'rewind' the offset/current block to it's original
 *  position without freeing memory so we can use it
 *  later
 */

typedef struct TempArena {
        Arena *a;
        ArenaBlock *saved_block;
        size_t saved_offset;
} TempArena;

TempArena init_temp_arena(Arena *a);
void free_temp_arena(TempArena t);
void destroy_arena(Arena *a);
