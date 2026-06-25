#pragma once

#include "../DSA/AST.h"
#include "../utils/arena_allocator.h"
#include <ncurses.h>
#include <stdbool.h>

// Serialise the current diagram (taken from global_objects directly for
// position data + the AST for command history) into a semicolon-delimited
// text file.
//
// filename  – destination path (relative or absolute).  The ".mcd" / ".mld"
//             extension is NOT added automatically; the caller (parse.c)
//             passes exactly what the user typed.
// diagram_type – MCD or MLD (from DiagramType enum in AST.h).
// console_win  – used for the y/n override prompt and success/error messages.
//
// Returns true on success, false on I/O error or user-cancelled override.
bool save_diagram(const char *filename, DiagramType diagram_type, WINDOW *console_win);

// Load a previously saved diagram file.
//
// filename    – path to the file (same rules as save_diagram).
// a           – arena used to slurp the file into memory.
// tree        – AST to populate; must already be initialised.
// console_win – forwarded to da_execute() for error display.
// needs_redraw – set to true if at least one command executes successfully.
//
// Returns true if the file was read and at least one command executed
// without a fatal error, false on I/O error or empty file.
bool load_diagram(const char *filename, Arena *a, AST *tree, WINDOW *console_win, bool *needs_redraw);
