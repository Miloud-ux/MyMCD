#ifndef PARSE_COMMANDS_H
#define PARSE_COMMANDS_H

#include "DSA/AST.h"
#include "utils/arena_allocator.h"
#include <ncurses.h>

bool da_execute(AST *t, Arena *a, WINDOW *console_win, const char *input, bool *needs_redraw);
void show_message(WINDOW *console_win, const char *msg);
#endif
