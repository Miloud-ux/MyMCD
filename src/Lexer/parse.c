#include "parse.h"
#include "../MCD_elements.h"
#include "../command_processor.h"
#include "ncurses.h"
#include <stdio.h>
#include <string.h>

void init_parser(Parser *p, const char *content) {
    if (!p) {
        return;
    }
    p->current = 0;
    p->count = 0;
    p->userInput = content;
}

void error_msg(WINDOW *console_win, Parser *p, const char *error) {
    int idx = p->current;
    if (idx >= p->count)
        idx = p->count - 1;
    if (idx < 0)
        idx = 0;

    wmove(console_win, 1, 1);
    wclrtoeol(console_win);

    wmove(console_win, 2, 1);
    wclrtoeol(console_win);

    wmove(console_win, 3, 1);
    wclrtoeol(console_win);

    wattron(console_win, COLOR_PAIR(1));
    mvwprintw(console_win, 1, 1, "[SYNTAX ERROR]: ");
    wattroff(console_win, COLOR_PAIR(1));
    mvwprintw(console_win, 1, 17, "%s", error);

    // Adding one for the carret since it's pointing to the last valid
    // token instead of pointing to the faulty token and getting an error
    mvwprintw(console_win, 2, 1, "> %s", p->userInput);
    for (int i = 0; i < p->tokens[idx].pos + 2; i++) {
        mvwaddch(console_win, 3, 2 + i, ' ');
    }
    wattron(console_win, COLOR_PAIR(1));
    mvwaddch(console_win, 3, 4 + p->tokens[idx].pos, '^');
    for (int i = 0; i < p->tokens[idx].length; i++) {
        mvwaddch(console_win, 3, 5 + p->tokens[idx].pos + i, '~');
    }
    wattroff(console_win, COLOR_PAIR(1));

    wrefresh(console_win);
}

void parse_command(Parser *p, const char *content, WINDOW *console_win) {
    // Root function that calls other child functions
    if (!content) {
        return;
    }
    init_parser(p, content);
    tokenize_content(content, p->tokens, &p->count);

    while (p->current < p->count) {
        Token t = peek_token(p->tokens, p->current);
        if (t.type == TOKEN_CREATE) {
            // consume token
            CreateCommand c = {0};
            advance_token(&p->current);
            if (parse_create(p, console_win, &c)) {
                execute_create(c);
            }
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

            const char *error = "Unknown command try : {create, help,clear}";
            error_msg(console_win, p, error);
            return;
        }
    }
}

bool parse_create(Parser *p, WINDOW *console, CreateCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_ENTITY) {
        advance_token(&p->current);
        if (parse_create_entity(console, p, false, c))
            return true;
    } else if (t.type == TOKEN_RELATIONSHIP) {
        advance_token(&p->current);
        if (parse_create_relationship(console, p, c))
            return true;
    } else {
        // Error expected : entity or relationship
        const char *error = "expected either Entity or relationship";
        error_msg(console, p, error);
    }
    return false;
}

// bool is_called is used to ignore "advance_token()" since we are already
// advancing from the caller.

bool parse_create_entity(WINDOW *console, Parser *p, bool is_called, CreateCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_STRING) {
        if (!is_called) {
            advance_token(&p->current);
        }
        c->type = TYPE_ENTITY;
        strncpy(c->Data.e.name, t.value, MAX_NAME_LEN);
        c->Data.e.name[MAX_NAME_LEN - 1] = '\0';
        return true;
    } else {
        const char *error = "expected entity name";
        error_msg(console, p, error);
    }
    return false;
}
// TODO : Add cardinality tokenization

bool parse_create_relationship(WINDOW *console, Parser *p, CreateCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_STRING) {
        // get entities and then add relationship
        advance_token(&p->current);
        bool Create_e1 = parse_create_entity(console, p, true, c);
        char e1_name[MAX_NAME_LEN];

        if (!Create_e1) {
            return false;
        }
        strncpy(e1_name, c->Data.e.name, MAX_NAME_LEN);
        e1_name[MAX_NAME_LEN - 1] = '\0';

        advance_token(&p->current);

        bool Create_e2 = parse_create_entity(console, p, true, c);
        char e2_name[MAX_NAME_LEN];

        if (!Create_e2) {
            return false;
        }

        // Both are succesful so we create the relationship
        strncpy(e2_name, c->Data.e.name, MAX_NAME_LEN);
        e2_name[MAX_NAME_LEN - 1] = '\0';
        advance_token(&p->current);

        c->type = TYPE_RELATIONSHIP;
        strncpy(c->Data.r.name, t.value, MAX_NAME_LEN);
        c->Data.e.name[MAX_NAME_LEN - 1] = '\0';

        strncpy(c->Data.r.e1_name, e1_name, MAX_NAME_LEN);
        c->Data.r.e1_name[MAX_NAME_LEN - 1] = '\0';

        strncpy(c->Data.r.e2_name, e2_name, MAX_NAME_LEN);
        c->Data.r.e2_name[MAX_NAME_LEN - 1] = '\0';

        return true;
    } else {
        const char *error = "expected Relationhip name";
        error_msg(console, p, error);
    }
    return false;
}

void execute_create(CreateCommand c) {
    if (c.type == TYPE_ENTITY) {
        createEntity(c.Data.e.name, 10, 10);
    } else {
        Entity *e1 = createEntity(c.Data.r.e1_name, 10, 10);
        Entity *e2 = createEntity(c.Data.r.e2_name, 10, 10);
        if (!e1 || !e2)
            return;
        addRelationship(10, 10, e1, e2, c.Data.r.name);
    }
}
