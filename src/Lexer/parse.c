#include "parse.h"
#include "../MCD_elements.h"
#include "../command_processor.h"
#include "ncurses.h"
#include <stdio.h>
#include <stdlib.h>
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
    // if (idx >= p->count)
    //     idx = p->count - 1;
    // if (idx < 0)
    //     idx = 0;

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
        // probabbly was stuck in an infine loop here (gotta print  this into a buffer to
        // confirm that it was printing infinite spaces and the issue isn't in the token
        // going out of bounds)
    }
    wattron(console_win, COLOR_PAIR(1));
    mvwaddch(console_win, 3, 3 + p->tokens[idx].pos, '^');
    for (int i = 0; i < p->tokens[idx].length; i++) {
        mvwaddch(console_win, 3, 4 + p->tokens[idx].pos + i, '~');
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
        } else if (t.type == TOKEN_ADD) {
            AddCommand c = {0};
            advance_token(&p->current);
            if (parse_add(p, console_win, &c)) {
                execute_addProperty(c);
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

bool parse_add(Parser *p, WINDOW *console, AddCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_PROPERTY) {
        advance_token(&p->current);
        if (parse_add_property(p, console, c))
            return true;
    } else {
        // Error expected : property name
        const char *error = "Expected property name";
        error_msg(console, p, error);
    }
    return false;
}

bool parse_add_property(Parser *p, WINDOW *console, AddCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_STRING) {
        // name of the entity/relationship
        strncpy(c->identifier_name, t.value, MAX_NAME_LEN); // check for empty string ?
        c->identifier_name[MAX_NAME_LEN - 1] = '\0';
        // Now get the actual property name and type:
        advance_token(&p->current);
        if (parse_add_property_name(p, console, c)) {
            return true;
        }
    } else {
        // Error expected : entity/relationship name
        const char *error = "Expected a valid existing entity/relationship name";
        error_msg(console, p, error);
    }
    return false;
}

bool parse_add_property_name(Parser *p, WINDOW *console, AddCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_STRING) {
        strncpy(c->prop_name, t.value, MAX_NAME_LEN);
        c->prop_name[MAX_NAME_LEN - 1] = '\0';
        advance_token(&p->current);
        if (parse_add_property_type(p, console, c)) {
            return true;
        }
    } else {
        // Error expected : Property name
        const char *error = "Expected a property name";
        error_msg(console, p, error);
    }
    return false;
}

bool parse_add_property_type(Parser *p, WINDOW *console, AddCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_IDENTIFIER) {
        strncpy(c->prop_type, t.value, MAX_TYPE_LEN);
        c->prop_type[MAX_TYPE_LEN - 1] = '\0';
        // needs better error handling
        return true;
    } else {
        // Error expected : Property type
        const char *error = "Expected a property type";
        error_msg(console, p, error);
    }
    return false;
}

Element *get_element_by_name(const char *name) {
    Element *el = malloc(sizeof(Element));
    if (!el) {
        return NULL;
    }

    // search in entities first
    Entity *e = search_entity(name);
    if (e) {
        el->type = TYPE_ENTITY;
        el->Element.e = e;
        return el;
    } else {
        // search in relationships
        // TODO: make entities and relationships not have the same name
        // because here it will add the property to the entity even
        // if a relationship exist with that name
        Relationship *r = search_relationship(name);
        el->type = TYPE_RELATIONSHIP;
        el->Element.r = r;
        return el;
    }
}
void execute_addProperty(AddCommand c) {
    Element *el = get_element_by_name(c.identifier_name);
    if (!el) {
        return;
    }

    if (el->type == TYPE_ENTITY) {
        addProperty(el->Element.e, c.prop_name, c.prop_type);
        // TODO: change return type for add proprety to boolean
        return;
    } else {
        addPropertyRelationship(el->Element.r, c.prop_name, c.prop_type);
        return;
    }
}
