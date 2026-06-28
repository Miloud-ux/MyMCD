#pragma once

#include "../DSA/AST.h"
#include "../utils/arena_allocator.h"
#include <ncurses.h>
#include <stdbool.h>
#include <stddef.h>

bool load_diagram(const char *filename, Arena *a, AST *tree, WINDOW *console_win, bool *needs_redraw);
bool save_diagram(const char *filename, DiagramType diagram_type, WINDOW *console_win);

// serialize the current diagram state into a caller-supplied buffer using
// the same text format as save_diagram.
// Returns the number of bytes written (excluding the null terminator).
// Works on both Windows and Linux.
size_t snapshot_diagram_to_buf(char *buf, size_t buf_size);
