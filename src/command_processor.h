#ifndef PARSE_COMMANDS_H
#define PARSE_COMMANDS_H

#include "MCD_elements.h"
#include <ncurses.h>

void show_message(WINDOW *console_win, const char *msg);

void execute_command(WINDOW *console_win, const char *input,
                     bool *needs_redraw);
#endif
