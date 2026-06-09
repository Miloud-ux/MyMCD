#include "arena_allocator.h"

// OS specefic headers for allocating a page
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <window.h>
#else
#include <sys/mman.h>
#endif

// memory alignment
#define DEFAULT_ALIGNMENT (2 * sizeof(void *)) // 16 bytes (for 64bit arch)

// useful macros
#define ARENA_PUSH_OBJECT(arena, type) (type *)arena_alloc_aligned(arena, sizeof(type), DEFAULT_ALIGNMENT)
#define ARENA_PUSH_ARRAY(arena, type, count) (type *)arena_alloc_aligned(arena, sizeof(type) * (count), DEFAULT_ALIGNMENT)

void *os_alloc(size_t size) {
#ifdef _WIN32
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
}

void os_free(void *ptr, size_t size) {
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, size);
#endif
}

void arena_init(Arena *a, size_t size) {
    if (!a) {
        return;
    }
    a->head = a->current = NULL;
    a->default_block_size = size;
}

ArenaBlock *arena_add_block(Arena *a, size_t size) {
    if (size == 0 || !a) {
        return NULL;
    }

    size_t size_to_alloc = (a->default_block_size < size) ? size : a->default_block_size;

    size_t total_size = sizeof(ArenaBlock) + size_to_alloc;
    void *mem = os_alloc(total_size);
    if (!mem) {
        // LOG: os failed to alloc mem
        return NULL;
    }

    ArenaBlock *new_block = (ArenaBlock *)mem;

    new_block->base = (uintptr_t)mem + sizeof(ArenaBlock);

    new_block->cap = size_to_alloc;
    new_block->next = NULL;
    new_block->offset = 0;

    if (a->current) {
        a->current->next = new_block;
    } else {
        a->head = new_block;
    }
    a->current = new_block;
    return new_block;
}

void *arena_alloc_aligned(Arena *a, size_t size, size_t align) {
    if (!a || size == 0 || align == 0) {
        return NULL;
    }

    if (!a->current) {
        if (!arena_add_block(a, size)) {
            return NULL;
        }
    }

    uintptr_t curr_ptr = a->current->base + a->current->offset;
    uintptr_t aligned_ptr = (curr_ptr + align - 1) & ~(align - 1);
    size_t padding = aligned_ptr - curr_ptr;

    if (a->current->offset + padding + size > a->current->cap) {
        if (!arena_add_block(a, size)) {
            return NULL;
        }

        curr_ptr = a->current->base + a->current->offset;
        aligned_ptr = (curr_ptr + align - 1) & ~(align - 1);
        padding = aligned_ptr - curr_ptr;
    }

    a->current->offset += (padding + size);
    return (void *)aligned_ptr;
}

// Temp arena
TempArena init_temp_arena(Arena *a) {
    if (!a) {
        return (TempArena){.a = NULL, .saved_block = NULL, .saved_block = 0};
    }

    return (TempArena){.a = a, .saved_block = a->current, .saved_offset = a->current->offset};
}

void free_temp_arena(TempArena t) {
    Arena *a = t.a;
    a->current = t.saved_block;
    if (a->current) {
        a->current->offset = t.saved_offset;
    }
}

void destroy_arena(Arena *a) {
    ArenaBlock *curr = a->current;
    while (curr) {
        ArenaBlock *next = curr->next;
        size_t total_size = sizeof(ArenaBlock) + curr->cap;
        os_free(curr, total_size);
        curr = next;
    }
    a->head = a->current = NULL;
}
