#include "parse.h"
#include "../MCD_elements.h"
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

    // create entity "John"

    while (p->current < p->count) {
        Token t = peek_token(p->tokens, p->current);
        if (t.type == TOKEN_CREATE) {
            // consume token
            advance_token(&p->current);
            parse_create(p, console_win);
        } else if (t.type == TOKEN_HELP) {
            // parse_help(p);
        } else if (t.type == TOKEN_CLEAR) {
            // parse_clear(p);
        } else if (t.type == TOKEN_EOF) {
            break;
        } else {
            // Unknown token
            // Maybe create an "error" struct to avoid passing console ?

            //  === debugging only ===
            // char debug_msg[256];
            // snprintf(debug_msg, sizeof(debug_msg),
            //          "Unknown token type = %d, value=%s", t.type, t.value);
            // show_message(console_win, debug_msg);

            // const char *error = "Unknown command try : {create, help,clear}";
            // error_msg(console_win, p->tokens[p->current].pos, error);
            return;
        }
    }
}

void parse_create(Parser *p, WINDOW *console) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_ENTITY) {
        advance_token(&p->current);
        parse_create_entity(console, p);
    } else if (t.type == TOKEN_RELATIONSHIP) {
        // advance_token(&p->current);
        // parse_create_relationship(Parser * p);
    } else {
        // Error expected : entity or relationship
        const char *error =
            "Syntax Error : expected either Entity or relationship";
        error_msg(console, p->current, error);
        return;
    }
}

void parse_create_entity(WINDOW *console, Parser *p) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_STRING) {
        advance_token(&p->current);
        createEntity(t.value, 10, 20);
    } else {
        const char *error = "Syntax Error : expected entity name";
        // TODO : change return type to bool to propagate and
        // remove the ghost effect of overwriting the errors
        error_msg(console, p->current, error);
    }
    return;
}
