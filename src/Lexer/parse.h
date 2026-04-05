#ifndef SRC_PARSE_H
#define SRC_PARSE_H
#include "tokenize.h"
#include <ncurses.h>

typedef struct {
        Token tokens[64];
        int count;
        int current;
} Parser;

void init_parser(Parser *p);
void parse_command(Parser *p, const char *content, WINDOW *console_win);
// maybe in future implementation of error msg function we can add a simple
// guess 'did you mean this "TOKEN"' then we have to pass the current token
void error_msg(WINDOW *console_win, int pos, const char *error);

#endif // !SRC_PARSE_H
