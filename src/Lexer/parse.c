#include "parse.h"
#include "../global_objects.h"
#include <ctype.h>
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
    int idx = p->current; // TODO: maybe -1 to track previous token but this might lead to out of bound error
    if (idx < 0) {        // TODO: needs further investigation
        idx = 0;
    } else if (idx == p->count) {

    } else {
        idx--;
    }

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

// Can be used to signal errors without the need to pass in the parser
void show_msg(WINDOW *console_win, const char *msg, const char *severity) {
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

    size_t len = strlen(severity);

    if (strlen(severity) == 0) {
        wattron(console_win, COLOR_PAIR(5));
        mvwprintw(console_win, 1, 1, "[GENERIC UPDATE]: ");
        wattroff(console_win, COLOR_PAIR(5));
        mvwprintw(console_win, 1, 5 + len, "%s !", msg);
    } else if (strcmp(severity, "WARNING") == 0) {
        wattron(console_win, COLOR_PAIR(3));
        mvwprintw(console_win, 1, 1, "[WARNING]: ");
        wattroff(console_win, COLOR_PAIR(3));
        mvwprintw(console_win, 1, 5 + len, "%s !", msg);
    } else if (strcmp(severity, "UPDATE") == 0) {
        wattron(console_win, COLOR_PAIR(5));
        mvwprintw(console_win, 1, 1, "[UPDATE]: ");
        wattroff(console_win, COLOR_PAIR(5));
        mvwprintw(console_win, 1, 5 + len, "%s !", msg);
    } else if (strcmp(severity, "INFO") == 0) {
        wattron(console_win, COLOR_PAIR(5));
        mvwprintw(console_win, 1, 1, "[INFO]: ");
        wattroff(console_win, COLOR_PAIR(5));
        mvwprintw(console_win, 1, 5 + len, "%s !", msg);
    } else if (strcmp(severity, "[LOGICAL ERROR]") == 0) {
        wattron(console_win, COLOR_PAIR(1));
        mvwprintw(console_win, 1, 1, "[LOGICAL ERROR]: ");
        wattroff(console_win, COLOR_PAIR(1));
        mvwprintw(console_win, 1, len + 5, "%s", msg);
    }

    wrefresh(console_win);
}

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
                if (execute_create(c)) {
                    // Add it to ast
                    Command *cmd = ARENA_PUSH_OBJECT(a, Command);
                    cmd->type = CREATE;
                    cmd->cmds.create_command = c;
                    add_ast_node(a, tree, cmd);
                } else {
                    error_msg_ex(console_win, p, ERR_RUNTIME, "Could not create the entity/relationship",
                                 "entity/relationship already exists or faced a memory issue");
                    break;
                }
            } else {
                return false;
            }
        } else if (t.type == TOKEN_ADD) {
            AddCommand c = {0};
            advance_token(&p->current);
            if (parse_add(p, console_win, &c)) {
                if (c.type == TYPE_PROPERTY) {
                    if (execute_addProperty(c, console_win)) {
                        // Sucess
                        Command *cmd = ARENA_PUSH_OBJECT(a, Command);
                        cmd->type = ADD;
                        cmd->cmds.add_command = c;
                        add_ast_node(a, tree, cmd);
                    } else {
                        error_msg_ex(console_win, p, ERR_RUNTIME, "Could not add the property",
                                     "entity/relationship not found, or property already exists");
                        break;
                    }
                } else if (c.type == TYPE_CARDINALITY) {
                    if (execute_addCardinality(c, console_win)) {
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
                                     "no relationship with that name, or (for the named-entity syntax) "
                                     "that entity isn't part of this relationship");
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
        } else if (t.type == TOKEN_CONVERT) {
            ConvertCommand c = {0};
            advance_token(&p->current);
            if (parse_convert(p, console_win, &c)) {
                if (c.type == MLD) {
                    if (convert_to_mld(console_win)) {
                        Command *cmd = ARENA_PUSH_OBJECT(a, Command);
                        cmd->type = CONVERT;
                        cmd->cmds.convert_command = c;
                        add_ast_node(a, tree, cmd);
                    }
                } else if (c.type == SQL) {
                    // convert_to_sql();
                }
            }

        } else if (t.type == TOKEN_CHANGE) {
            ChangeNameCommand c = {0};
            advance_token(&p->current);
            if (parse_change_name(p, console_win, &c)) {
                if (execute_changeName(c, console_win)) {
                    Command *cmd = ARENA_PUSH_OBJECT(a, Command);
                    cmd->type = CHANGE_NAME;
                    cmd->cmds.change_name_command = c;
                    add_ast_node(a, tree, cmd);
                    show_msg(console_win, "Name changed succesfully", "UPDATE");
                } else {
                    error_msg_ex(console_win, p, ERR_RUNTIME, "Could not change the name",
                                 "no entity/relationship with that name, or the new name is already taken");
                    break;
                }
            } else {
                return false;
            }

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

        if (strlen(t.value) == 0) {
            const char *error = "Empty Entity name";
            error_msg(console, p, error);
            return false;
        } else if (!valid_name(t.value)) {
            const char *error = "Invalid Entity name (use alphanumerical characters only)";
            error_msg(console, p, error);
            return false;
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

bool parse_create_relationship(WINDOW *console, Parser *p, CreateCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_STRING) {
        // get entities and then add relationship

        if (strlen(t.value) == 0) {
            const char *error = "Empty relationship  name";
            error_msg(console, p, error);
            return false;
        } else if (!valid_name(t.value)) {
            const char *error = "Invalid relationship name (use alphanumerical characters only)";
            error_msg(console, p, error);
            return false;
        }

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
        c->Data.r.name[MAX_NAME_LEN - 1] = '\0';

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

bool execute_create(CreateCommand c) {
    static int x = 10;
    static int y = 10;

    if (c.type == TYPE_ENTITY) {
        if (!search_entity(c.Data.e.name)) {
            createEntity(c.Data.e.name, x, y);
            x = y += 10;
            return true;
        }
    } else {
        Entity *e1 = search_entity(c.Data.r.e1_name), *e2 = search_entity(c.Data.r.e2_name);
        if (!e1) {
            e1 = createEntity(c.Data.r.e1_name, x, y);
            x = y += 10;
        }
        if (!e2) {
            e2 = createEntity(c.Data.r.e2_name, x, y);
            x = y += 10;
        }
        // allocation failed
        if (!e1 || !e2)
            return false;

        if (!search_relationship(c.Data.r.name)) {
            addRelationship(10, 10, e1, e2, c.Data.r.name);
            return true;
        }
    }
    return false;
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

        if (parse_add_property_key(p, console, c)) {
            return true;
        } else {
            return false; // avoid overridng the original error msg
        }

    default:
        const char *error = "Expected a property type(money, str, date, int, double)";
        error_msg(console, p, error);
        return false;
    }
}

bool parse_add_property_key(Parser *p, WINDOW *console, AddCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    switch (t.type) {
    case TOKEN_KEY:
        if (strcmp(t.value, "pk") == 0) {
            c->Data.p.type = PRIMARY_KEY;
            advance_token(&p->current);
            return true;
        } else if (strcmp(t.value, "fk") == 0) {
            c->Data.p.type = FOREIGN_KEY;
            advance_token(&p->current);
            return true;
        }

    case TOKEN_EOF: // no key provided treat is as a default property
        c->Data.p.type = NORMAL_KEY;
        advance_token(&p->current);
        return true;

    default:
        const char *error = "Expected Key type (pk, fk or none )";
        error_msg(console, p, error);
        return false;
    }
}

// Doesn't search for properties or other element types
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

        int comma_count = 0;
        for (int i = 0; t.value[i] != '\0'; i++) {
            if (t.value[i] == ',') {
                comma_count++;
            }
        }

        if (comma_count == 3) {
            c->Data.c.entity_name[0] = '\0';
            strncpy(c->Data.c.value, t.value, RAW_CARDINALITY_LEN); // "1,n,n,0"
            c->Data.c.value[RAW_CARDINALITY_LEN - 1] = '\0';
            advance_token(&p->current);
            return true;
        }

        strncpy(c->Data.c.entity_name, t.value, MAX_NAME_LEN);
        c->Data.c.entity_name[MAX_NAME_LEN - 1] = '\0';
        advance_token(&p->current);

        Token t2 = peek_token(p->tokens, p->current);
        if (t2.type == TOKEN_STRING && strlen(t2.value) > 0) {
            strncpy(c->Data.c.value, t2.value, RAW_CARDINALITY_LEN);
            c->Data.c.value[RAW_CARDINALITY_LEN - 1] = '\0';
            advance_token(&p->current);
            return true;
        } else {
            const char *error = "Expected that entity's cardinality value example: (\"1,n\")";
            error_msg(console, p, error);
            return false;
        }

    } else {
        const char *error = "Expected cardinality value example: (\"1,n,0,1\"). type '\\help' for more details";
        error_msg(console, p, error);
        return false;
    }
}

bool execute_addProperty(AddCommand c, WINDOW *console_win) {
    Element *el = get_element_by_name(c.identifier_name);
    if (!el) {
        return false;
    }

    KeyType key = c.Data.p.type;
    if (key == NORMAL_KEY) {

        if (el->type == TYPE_ENTITY) {
            if (addProperty(el->Element.e, c.Data.p.prop_name, c.Data.p.prop_type, NORMAL_KEY)) {
                free(el);
                return true;
            }
        } else {
            if (addPropertyRelationship(el->Element.r, c.Data.p.prop_name, c.Data.p.prop_type, NORMAL_KEY)) {
                free(el);
                return true;
            }
        }
    } else if (key == PRIMARY_KEY) { // since foreign keys are only added during convertion to MLD
        if (el->type == TYPE_ENTITY) {
            if (addProperty(el->Element.e, c.Data.p.prop_name, c.Data.p.prop_type, PRIMARY_KEY)) {
                free(el);
                return true;
            }
        } else {
            if (addPropertyRelationship(el->Element.r, c.Data.p.prop_name, c.Data.p.prop_type, PRIMARY_KEY)) {
                free(el);
                return true;
            }
        }

    } else { // Foreign key but give a warning to the user he is violating MCD standards
        if (global_objects.current_dtype == MCD) {
            show_msg(console_win, "Foreign keys aren't allowed in MCD diagrams, convert to MLD first", "WARNING");
            free(el);
            return true;
        } else {
            // Add foreign key to MLD (incase user want to do so...)
            show_msg(console_win, "Adding a foreign key to a MLD diagram", "WARNING");
            if (el->type == TYPE_ENTITY) {
                if (addProperty(el->Element.e, c.Data.p.prop_name, c.Data.p.prop_type, FOREIGN_KEY)) {
                    free(el);
                    return true;
                }
            } else {
                if (addPropertyRelationship(el->Element.r, c.Data.p.prop_name, c.Data.p.prop_type, FOREIGN_KEY)) {
                    free(el);
                    return true;
                }
            }
        }
    }
    free(el);
    return false;
}

bool execute_addCardinality(AddCommand c, WINDOW *console_win) {
    Relationship *r = search_relationship(c.identifier_name);
    if (!r) {
        // NO relationship found
        return false;
    }

    if (strlen(c.Data.c.entity_name) == 0) {
        if (addCardinalityAPI(c.Data.c.value, r)) {
            char msg[128];
            const char *e1_name = r->e1 ? r->e1->name : "?";
            const char *e2_name = r->e2 ? r->e2->name : "?";
            snprintf(msg, sizeof(msg), "First cardinality belongs to %s, second belongs to %s", e1_name, e2_name);
            show_msg(console_win, msg, "INFO");
            return true;
        }
        return false;
    }

    return addCardinalityForEntity(c.Data.c.entity_name, c.Data.c.value, r);
}

bool parse_change_name(Parser *p, WINDOW *console, ChangeNameCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type != TOKEN_NAME) {
        const char *error = "Expected 'name' after change";
        error_msg(console, p, error);
        return false;
    }
    advance_token(&p->current);

    t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_STRING) {
        if (strlen(t.value) == 0) {
            const char *error = "Empty entity/relationship name";
            error_msg(console, p, error);
            return false;
        }
        strncpy(c->old_name, t.value, MAX_NAME_LEN);
        c->old_name[MAX_NAME_LEN - 1] = '\0';
        advance_token(&p->current);

        if (parse_change_name_value(p, console, c)) {
            return true;
        }
    } else {
        const char *error = "Expected the current entity/relationship name";
        error_msg(console, p, error);
    }
    return false;
}

bool parse_change_name_value(Parser *p, WINDOW *console, ChangeNameCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_STRING) {
        if (strlen(t.value) == 0) {
            const char *error = "Empty new name";
            error_msg(console, p, error);
            return false;
        }
        strncpy(c->new_name, t.value, MAX_NAME_LEN);
        c->new_name[MAX_NAME_LEN - 1] = '\0';
        advance_token(&p->current);
        return true;
    } else {
        const char *error = "Expected the new name";
        error_msg(console, p, error);
    }
    return false;
}

bool execute_changeName(ChangeNameCommand c, WINDOW *console_win) {
    Entity *e = search_entity(c.old_name);
    Relationship *r = NULL;
    if (!e) {
        r = search_relationship(c.old_name);
    }
    if (!e && !r) {
        return false;
    }

    Entity *clash_e = search_entity(c.new_name);
    Relationship *clash_r = search_relationship(c.new_name);
    bool clashes = (clash_e && clash_e != e) || (clash_r && clash_r != r);
    if (clashes) {
        show_msg(console_win, "An entity or relationship with that name already exists", "WARNING");
        return false;
    }

    if (e) {
        strncpy(e->name, c.new_name, MAX_NAME_LEN);
        e->name[MAX_NAME_LEN - 1] = '\0';
    } else {
        strncpy(r->name, c.new_name, MAX_NAME_LEN);
        r->name[MAX_NAME_LEN - 1] = '\0';
    }
    return true;
}

bool parse_convert(Parser *p, WINDOW *win, ConvertCommand *c) {
    Token t = peek_token(p->tokens, p->current);

    if (t.type == TOKEN_MLD) {
    }

    switch (t.type) {
    case TOKEN_MLD:
        c->type = MLD;
        advance_token(&p->current);
        return true;
    case TOKEN_SQL:
        c->type = SQL;
        advance_token(&p->current);
        return true;
    default:
        const char *error = "Expected a valid conversion mode (MLD, SQL)";
        error_msg(win, p, error);
        return false;
    }
}

static void migrate_foreign_key(Entity *dst, Entity *src, Relationship *r) {
    for (int i = 0; i < src->num_properties; i++) {
        if (src->properties[i] && src->properties[i]->keytype == PRIMARY_KEY) {
            addProperty(dst, src->properties[i]->name, src->properties[i]->type, FOREIGN_KEY);
        }
    }
    for (int i = 0; i < r->num_properties; i++) {
        if (r->properties[i]) {
            addProperty(dst, r->properties[i]->name, r->properties[i]->type, r->properties[i]->keytype);
        }
    }
}

static bool create_junction_entity(Relationship *r, Entity *e1, Entity *e2) {
    char junction_name[MAX_NAME_LEN];
    snprintf(junction_name, MAX_NAME_LEN, "%s_%s", e1->name, e2->name);

    if (search_entity(junction_name) || search_relationship(junction_name)) {
        return false;
    }

    Entity *junction = createEntity(junction_name, r->x, r->y);
    if (!junction) {
        return false;
    }

    for (int i = 0; i < e1->num_properties; i++) {
        if (e1->properties[i] && e1->properties[i]->keytype == PRIMARY_KEY) {
            addProperty(junction, e1->properties[i]->name, e1->properties[i]->type, PRIMARY_KEY);
        }
    }
    for (int i = 0; i < e2->num_properties; i++) {
        if (e2->properties[i] && e2->properties[i]->keytype == PRIMARY_KEY) {
            addProperty(junction, e2->properties[i]->name, e2->properties[i]->type, PRIMARY_KEY);
        }
    }
    for (int i = 0; i < r->num_properties; i++) {
        if (r->properties[i]) {
            addProperty(junction, r->properties[i]->name, r->properties[i]->type, r->properties[i]->keytype);
        }
    }
    return true;
}

bool convert_to_mld(WINDOW *console_win) {
    bool had_skipped = false;

    for (int i = 0; i < global_objects.relationship_count; i++) {
        Relationship *r = global_objects.relationships[i];
        if (!r) {
            continue;
        }
        if (!r->cards[0] || !r->cards[1]) {
            had_skipped = true;
            continue;
        }

        char max1 = r->cards[0]->value[2];
        char max2 = r->cards[1]->value[2];

        if ((max1 != '1' && max1 != 'n') || (max2 != '1' && max2 != 'n')) {
            had_skipped = true;
            continue;
        }

        if (max1 == 'n' && max2 == 'n') {
            if (!create_junction_entity(r, r->e1, r->e2)) {
                had_skipped = true;
                continue;
            }
            unregister_relationship(r);
        } else if (max1 == '1' && max2 == 'n') {
            migrate_foreign_key(r->e1, r->e2, r);
            unregister_relationship(r);
        } else if (max1 == 'n' && max2 == '1') {
            migrate_foreign_key(r->e2, r->e1, r);
            unregister_relationship(r);
        } else {
            char card1_min = r->cards[0]->value[0];
            char card2_min = r->cards[1]->value[0];
            if (card1_min == '1') {
                migrate_foreign_key(r->e1, r->e2, r);
            } else if (card2_min == '1') {
                migrate_foreign_key(r->e2, r->e1, r);
            } else {
                migrate_foreign_key(r->e1, r->e2, r);
            }
            unregister_relationship(r);
        }
    }

    global_objects.current_dtype = MLD;

    if (had_skipped) {
        show_msg(console_win, "Converted to MLD, but some relationships were skipped (missing/invalid cardinality)",
                 "WARNING");
    } else {
        show_msg(console_win, "Converted to MLD succesfully", "UPDATE");
    }

    return true;
}

// simple check for input
bool valid_name(const char *name) {
    for (size_t i = 0; i < strlen(name); i++) {
        if (!isalnum(name[i])) {
            return false;
        }
    }

    return true;
}
