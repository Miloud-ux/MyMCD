#include "undo_stack.h"
#include <stdlib.h>
#include <string.h>

// Frees any heap snapshot owned by an entry.  Safe to call on entries whose
// snapshot was never allocated (snapshot is NULL).
void undo_entry_free(UndoEntry *e) {
    if (!e)
        return;
    if (e->type == UNDO_CONVERT_MLD || e->type == UNDO_CLEAR) {
        free(e->data.convert_mld.snapshot);
        e->data.convert_mld.snapshot = NULL;
        e->data.convert_mld.snapshot_len = 0;
    }
}

// Zero-initialises a stack.  Must be safe on uninitialised memory, so it
// never inspects entries.
void undo_stack_init(UndoStack *s) {
    if (!s)
        return;
    memset(s, 0, sizeof(*s));
    s->top = -1;
}

// Frees the snapshots of all currently-pushed entries and resets the stack.
// Only call on a stack that is known to be valid (i.e. was initialised and
// only modified through the undo_stack API).
void undo_stack_clear(UndoStack *s) {
    if (!s)
        return;
    for (int i = 0; i < UNDO_STACK_CAPACITY; i++) {
        undo_entry_free(&s->entries[i]);
    }
    undo_stack_init(s);
}

bool undo_stack_push(UndoStack *s, UndoEntry entry) {
    if (s->top >= UNDO_STACK_CAPACITY - 1) {
        // stack full drop the oldest entry by shifting everything down
        // so the user always gets the most recent UNDO_STACK_CAPACITY ops
        undo_entry_free(&s->entries[0]);
        for (int i = 0; i < UNDO_STACK_CAPACITY - 1; i++) {
            s->entries[i] = s->entries[i + 1];
        }
        s->entries[UNDO_STACK_CAPACITY - 1] = entry;
        return true;
    }
    s->entries[++s->top] = entry;
    return true;
}

bool undo_stack_pop(UndoStack *s, UndoEntry *out) {
    if (s->top < 0) {
        return false;
    }
    *out = s->entries[s->top];
    // Ownership of the snapshot (if any) transfers to the caller; the slot
    // must not free it again.
    s->entries[s->top].data.convert_mld.snapshot = NULL;
    s->entries[s->top].data.convert_mld.snapshot_len = 0;
    s->top--;
    return true;
}

bool undo_stack_is_empty(const UndoStack *s) { return s->top < 0; }
