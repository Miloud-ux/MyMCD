#include "parse.h"
#include "../command_processor.h"
#include "ncurses.h"
#include <stdio.h>

void init_parser(Parser *p) {
    if (!p) {
        return;
    }
    p->current = 0;
    p->count = 0;
}

void error_msg(WINDOW *console_win, int pos, const char *error) {
    show_message(console_win, error);
}

void parse_command(Parser *p, const char *content, WINDOW *console_win) {
    // Root function that calls other child functions
    if (!content) {
        return;
    }
    init_parser(p);
    tokenize_content(content, p->tokens, &p->count);

    Token t = get_next_token(p->tokens, &p->current, p->count);

    while (t.type != TOKEN_EOF) {
        if (t.type == TOKEN_CREATE) {
            parse_create(p);
        } else if (t.type == TOKEN_HELP) {
            parse_help(p);
        } else if (t.type == TOKEN_CLEAR) {
            parse_clear(p);
        } else {
            // Unknown token
            // Maybe create an "error" struct to avoid passing console ?
            const char *error = "Unknown command try : {create, help, clear}";
            error_msg(console_win, p->tokens[p->current].pos, error);
        }
        t = get_next_token(p->tokens, &p->current, p->count);
    }
}
