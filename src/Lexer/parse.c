#include "parse.h"
#include "../global_objects.h"
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

// Update => New function: error_msg_ex(). Same 3-line console layout/style as
// error_msg() above (still "> <input>" + "^~~~" pointing at a token), but used
// for commands that parsed fine and only failed once we tried to *run* them
// (e.g. "add card" on a relationship that doesn't exist). It labels the line
// [RUNTIME ERROR] instead of [SYNTAX ERROR], and appends a short "why" hint in
// parentheses so the message is actually useful instead of "for some reason ??".
// error_msg() itself was left completely unchanged.
void error_msg_ex(WINDOW *console_win, Parser *p, ErrorKind kind, const char *error, const char *hint) {
    int idx = p->current;

    char full_msg[200];
    if (hint && hint[0] != '\0') {
        snprintf(full_msg, sizeof(full_msg), "%s (%s)", error, hint);
    } else {
        snprintf(full_msg, sizeof(full_msg), "%s", error);
    }

    wmove(console_win, 1, 1);
    wclrtoeol(console_win);

    wmove(console_win, 2, 1);
    wclrtoeol(console_win);

    wmove(console_win, 3, 1);
    wclrtoeol(console_win);

    const char *label = (kind == ERR_RUNTIME) ? "[RUNTIME ERROR]: " : "[SYNTAX ERROR]: ";

    wattron(console_win, COLOR_PAIR(1));
    mvwprintw(console_win, 1, 1, "%s", label);
    wattroff(console_win, COLOR_PAIR(1));
    mvwprintw(console_win, 1, (int)strlen(label) + 1, "%s", full_msg);

    // Same caret/underline mechanic as error_msg(). Note: for a runtime error,
    // p->current has already advanced past the whole command (it points at
    // whatever comes next, often EOF), so the "^~~~" will land near the end of
    // the line rather than exactly under the offending name (e.g. the bad
    // relationship name). Pinpointing the exact token would need the Parser to
    // remember which token index held the identifier, which isn't tracked
    // today — left as-is to avoid touching the Parser/Token structures.
    mvwprintw(console_win, 2, 1, "> %s", p->userInput);
    for (int i = 0; i < p->tokens[idx].pos + 2; i++) {
        mvwaddch(console_win, 3, 2 + i, ' ');
    }
    wattron(console_win, COLOR_PAIR(1));
    mvwaddch(console_win, 3, 3 + p->tokens[idx].pos, '^');
    for (int i = 0; i < p->tokens[idx].length; i++) {
        mvwaddch(console_win, 3, 4 + p->tokens[idx].pos + i, '~');
    }
    wattroff(console_win, COLOR_PAIR(1));

    wrefresh(console_win);
}

bool parse_command(AST *tree, Parser *p, const char *content, WINDOW *console_win, Arena *a) {
    // Root function that calls other child functions
    if (!content) {
        const char *error = "Error Parsing Command (String doesn't exist)";
        error_msg(console_win, p, error);
        return false;
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
                // Add it to ast
                Command *cmd = ARENA_PUSH_OBJECT(a, Command);
                cmd->type = CREATE;
                cmd->cmds.create_command = c;
                add_ast_node(a, tree, cmd);
            }
        } else if (t.type == TOKEN_ADD) {
            AddCommand c = {0};
            advance_token(&p->current);
            if (parse_add(p, console_win, &c)) {
                if (c.type == TYPE_PROPERTY) {
                    if (execute_addProperty(c)) {
                        // Sucess
                        Command *cmd = ARENA_PUSH_OBJECT(a, Command);
                        cmd->type = ADD;
                        cmd->cmds.add_command = c;
                        add_ast_node(a, tree, cmd);
                    } else {
                        // Update => replaced the generic "for some reason" message
                        // with error_msg_ex(): labels this a runtime error and adds a
                        // hint. Most likely cause is c.identifier_name not matching
                        // any existing entity/relationship, since get_element_by_name()
                        // is what execute_addProperty() fails on first.
                        error_msg_ex(console_win, p, ERR_RUNTIME, "Could not add the property",
                                     "entity/relationship not found, or property already exists");
                        break;
                    }
                } else if (c.type == TYPE_CARDINALITY) {
                    if (execute_addCardinality(c)) {
                        // Sucess
                        Command *cmd = ARENA_PUSH_OBJECT(a, Command);
                        cmd->type = ADD;
                        cmd->cmds.add_command = c;
                        add_ast_node(a, tree, cmd);
                    } else {
                        // Update => replaced the generic "for some reason ??" message
                        // with error_msg_ex(): labels this a runtime error and adds a
                        // hint. This branch fires when search_relationship(c.identifier_name)
                        // finds nothing, i.e. no relationship with that name exists yet.
                        error_msg_ex(console_win, p, ERR_RUNTIME, "Could not add the cardinality",
                                     "no relationship with that name - check spelling or create it first");
                        break;
                    }
                } else {
                    const char *error = "Unknown Add command try (Add property or Add cardinality )";
                    error_msg(console_win, p, error);
                    break;
                }

            } else {
                // parse_add() already alerted this error
                return false;
            }
        } else if (t.type == TOKEN_HELP) {
            // Handled in main
        } else if (t.type == TOKEN_CLEAR) {
            advance_token(&p->current);
            parse_clear();
        } else if (t.type == TOKEN_EOF) {
            break;
        } else if (t.type == TOKEN_UNKNOWN) {
            // Unknown token
            // Maybe create an "error" struct to avoid passing console ?

            //== = debugging only == =
            char debug_msg[256];
            snprintf(debug_msg, sizeof(debug_msg), "Unknown token type = %d, value=%s", t.type, t.value);
            error_msg(console_win, p, debug_msg);

            return false;
        } else {
            const char *error = "Unknown command try : {create, help,clear}";
            error_msg(console_win, p, error);
            return false;
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
        if (!search_entity(c.Data.e.name)) {
            createEntity(c.Data.e.name, 10, 10);
        }
    } else {
        Entity *e1 = search_entity(c.Data.r.e1_name), *e2 = search_entity(c.Data.r.e2_name);
        if (!e1) {
            e1 = createEntity(c.Data.r.e1_name, 10, 10);
        }
        if (!e2) {
            e2 = createEntity(c.Data.r.e2_name, 10, 10);
        }
        // allocation failed
        if (!e1 || !e2)
            return;
        addRelationship(10, 10, e1, e2, c.Data.r.name);
    }
}

bool parse_add(Parser *p, WINDOW *console, AddCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_PROPERTY) {
        c->type = TYPE_PROPERTY;
        advance_token(&p->current);
        if (parse_add_property(p, console, c))
            return true;
    } else if (t.type == TOKEN_CARDINALITY) {
        c->type = TYPE_CARDINALITY;
        advance_token(&p->current);
        if (parse_add_cardinality(p, console, c))
            return true;
    } else {
        const char *error = "Expected either property or card (type '\\help' for more info)";
        error_msg(console, p, error);
    }
    return false;
}

bool parse_add_property(Parser *p, WINDOW *console, AddCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_STRING) {
        // name of the entity/relationship

        if (strlen(t.value) == 0) {
            const char *error = "Empty Entity/Relationship name";
            error_msg(console, p, error);
            return false;
        }
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
        if (strlen(t.value) == 0) {
            const char *error = "Empty property name";
            error_msg(console, p, error);
            return false;
        }

        strncpy(c->Data.p.prop_name, t.value, MAX_NAME_LEN);
        c->Data.p.prop_name[MAX_NAME_LEN - 1] = '\0';

        advance_token(&p->current);

        if (parse_add_property_type(p, console, c)) {
            return true;
        }
    } else {
        const char *error = "Expected a property name";
        error_msg(console, p, error);
    }
    return false;
}

bool parse_add_property_type(Parser *p, WINDOW *console, AddCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    switch (t.type) {
    case TOKEN_INT_TYPE:
    case TOKEN_STRING_TYPE:
    case TOKEN_DOUBLE_TYPE:
    case TOKEN_DATE_TYPE:
    case TOKEN_MONEY_TYPE:
        strncpy(c->Data.p.prop_type, t.value, MAX_TYPE_LEN);
        c->Data.p.prop_type[MAX_TYPE_LEN - 1] = '\0';
        advance_token(&p->current);
        return true;
    default:
        const char *error = "Expected a property type(money, str, date, int, double)";
        error_msg(console, p, error);
        return false;
    }
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
        if (!r) {
            return NULL;
        }
        el->type = TYPE_RELATIONSHIP;
        el->Element.r = r;
        return el;
    }
}

void parse_clear() { init_global_objects(); }

bool parse_add_cardinality(Parser *p, WINDOW *console, AddCommand *c) {
    Token t = peek_token(p->tokens, p->current);

    if (t.type == TOKEN_STRING) {
        if (strlen(t.value) == 0) {
            const char *error = "Please enter a valid ralationship name ";
            error_msg(console, p, error);
            return false;
        }

        strncpy(c->Data.c.r.name, t.value, MAX_NAME_LEN);
        c->Data.c.r.name[MAX_NAME_LEN - 1] = '\0';

        // Update => this was the actual bug: execute_addCardinality() looks up the
        // relationship using c.identifier_name, but only Data.c.r.name was being set
        // here. identifier_name stayed "" (zero-initialized from "AddCommand c = {0}"
        // in parse_command), so search_relationship("") always returned NULL and the
        // command always failed with "Failed to execute Add card command for some
        // reason ??", no matter what name you typed.
        strncpy(c->identifier_name, t.value, MAX_NAME_LEN);
        c->identifier_name[MAX_NAME_LEN - 1] = '\0';

        advance_token(&p->current);

        if (parse_add_cardinality_value(p, console, c)) {
            return true;
        }

    } else {
        const char *error = "Expected A relationship name (\"r_name\")";
        error_msg(console, p, error);
        return false;
    }
    return false;
}

bool parse_add_cardinality_value(Parser *p, WINDOW *console, AddCommand *c) {
    Token t = peek_token(p->tokens, p->current);

    if (t.type == TOKEN_STRING) {
        if (strlen(t.value) == 0) {
            const char *error = "Please enter a valid cardinality value (0,1,n)";
            error_msg(console, p, error);
            return false;
        }

        strncpy(c->Data.c.value, t.value, RAW_CARDINALITY_LEN); // "1,n,n,0"
        c->Data.c.value[RAW_CARDINALITY_LEN - 1] = '\0';
        advance_token(&p->current);

        return true;

    } else {
        const char *error = "Expected cardinality value example: (\"1,n,0,1\"). type '\\help' for more details";
        error_msg(console, p, error);
        return false;
    }
}

bool execute_addProperty(AddCommand c) {
    Element *el = get_element_by_name(c.identifier_name);
    if (!el) {
        return false;
    }

    if (el->type == TYPE_ENTITY) {
        if (addProperty(el->Element.e, c.Data.p.prop_name, c.Data.p.prop_type)) {
            free(el);
            return true;
        }
    } else {
        if (addPropertyRelationship(el->Element.r, c.Data.p.prop_name, c.Data.p.prop_type)) {
            free(el);
            return true;
        }
    }
    free(el);
    return false;
}

bool execute_addCardinality(AddCommand c) {
    Relationship *r = search_relationship(c.identifier_name);
    if (!r) {
        // NO relationship found
        return false;
    }

    if (addCardinalityAPI(c.Data.c.value, r)) {
        return true;
    }

    return false;
}
