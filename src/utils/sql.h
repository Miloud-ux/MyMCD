#pragma once

#include <ncurses.h>
#include <stdbool.h>

bool generate_sql(const char *filename, WINDOW *console_win);
