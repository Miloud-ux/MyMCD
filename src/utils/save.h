#pragma once

#include "../DSA/AST.h"
#include "../utils/arena_allocator.h"
#include <ncurses.h>
#include <stdbool.h>

bool load_diagram(const char *filename, Arena *a, AST *tree, WINDOW *console_win, bool *needs_redraw);
