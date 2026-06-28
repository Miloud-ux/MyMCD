#include "undo_stack.h"

void undo_stack_init(UndoStack *s) { s->top = -1; }

bool undo_stack_push(UndoStack *s, UndoEntry entry) {
    if (s->top >= UNDO_STACK_CAPACITY - 1) {
        // stack full drop the oldest entry by shifting everything down
        // so the user always gets the most recent UNDO_STACK_CAPACITY ops
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
    *out = s->entries[s->top--];
    return true;
}

bool undo_stack_is_empty(const UndoStack *s) { return s->top < 0; }
