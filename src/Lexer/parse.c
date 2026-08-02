#include "parse.h"
#include "../global_objects.h"
#include "../utils/save.h"
#include "../utils/sql.h"
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

                    // Push the inverse onto the undo stack so the user can
                    // undo this creation by deleting the entity/relationship
                    UndoEntry ue = {0};
                    if (c.type == TYPE_ENTITY) {
                        ue.type = UNDO_CREATE_ENTITY;
                        strncpy(ue.data.create_entity.name, c.Data.e.name, MAX_NAME_LEN - 1);
                    } else {
                        ue.type = UNDO_CREATE_RELATIONSHIP;
                        strncpy(ue.data.create_rel.name, c.Data.r.name, MAX_NAME_LEN - 1);
                    }
                    undo_stack_push(&global_objects.undo_stack, ue);
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

                        // Record which element type this property was added to
                        // so execute_undo knows whether to search entities or
                        // relationships when removing it
                        Element *el = get_element_by_name(c.identifier_name);
                        UndoEntry ue = {0};
                        ue.type = UNDO_ADD_PROP;
                        strncpy(ue.data.add_prop.identifier_name, c.identifier_name, MAX_NAME_LEN - 1);
                        strncpy(ue.data.add_prop.prop_name, c.Data.p.prop_name, MAX_NAME_LEN - 1);
                        strncpy(ue.data.add_prop.prop_type, c.Data.p.prop_type, MAX_TYPE_LEN - 1);
                        ue.data.add_prop.keytype = (int)c.Data.p.type;
                        ue.data.add_prop.is_relationship = (el && el->type == TYPE_RELATIONSHIP);
                        if (el)
                            free(el);
                        undo_stack_push(&global_objects.undo_stack, ue);
                    } else {
                        error_msg_ex(console_win, p, ERR_RUNTIME, "Could not add the property",
                                     "entity/relationship not found, or property already exists");
                        break;
                    }
                } else if (c.type == TYPE_CARDINALITY) {
                    // Capture the cardinalities that are about to be overwritten
                    // before calling execute_addCardinality so the undo entry
                    // stores the previous state, not the new one
                    Relationship *r_before = search_relationship(c.identifier_name);
                    UndoEntry ue = {0};
                    ue.type = UNDO_ADD_CARD;
                    strncpy(ue.data.add_card.rel_name, c.identifier_name, MAX_NAME_LEN - 1);
                    if (r_before && r_before->cards[0]) {
                        ue.data.add_card.had_card0 = true;
                        strncpy(ue.data.add_card.card0, r_before->cards[0]->value, CARDINALITY_LEN - 1);
                    }
                    if (r_before && r_before->cards[1]) {
                        ue.data.add_card.had_card1 = true;
                        strncpy(ue.data.add_card.card1, r_before->cards[1]->value, CARDINALITY_LEN - 1);
                    }

                    if (execute_addCardinality(c, console_win)) {
                        // Sucess
                        Command *cmd = ARENA_PUSH_OBJECT(a, Command);
                        cmd->type = ADD;
                        cmd->cmds.add_command = c;
                        add_ast_node(a, tree, cmd);

                        undo_stack_push(&global_objects.undo_stack, ue);
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
                    // Take a full snapshot of the MCD state before converting
                    // so the user can restore it with undo.  snapshot_diagram_to_buf
                    // writes into a heap buffer so the UndoEntry stays small.
                    UndoEntry ue = {0};
                    ue.type = UNDO_CONVERT_MLD;
                    ue.data.convert_mld.snapshot = malloc(UNDO_SNAPSHOT_SIZE);
                    if (ue.data.convert_mld.snapshot) {
                        ue.data.convert_mld.snapshot_len =
                            snapshot_diagram_to_buf(ue.data.convert_mld.snapshot, UNDO_SNAPSHOT_SIZE);
                    }
                    undo_stack_push(&global_objects.undo_stack, ue);

                    if (convert_to_mld(console_win)) {
                        Command *cmd = ARENA_PUSH_OBJECT(a, Command);
                        cmd->type = CONVERT;
                        cmd->cmds.convert_command = c;
                        add_ast_node(a, tree, cmd);
                    }
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

                    // The inverse of "change name A -> B" is "change name B -> A"
                    UndoEntry ue = {0};
                    ue.type = UNDO_CHANGE_NAME;
                    strncpy(ue.data.change_name.old_name, c.old_name, MAX_NAME_LEN - 1);
                    strncpy(ue.data.change_name.new_name, c.new_name, MAX_NAME_LEN - 1);
                    undo_stack_push(&global_objects.undo_stack, ue);
                } else {
                    error_msg_ex(console_win, p, ERR_RUNTIME, "Could not change the name",
                                 "no entity/relationship with that name, or the new name is already taken");
                    break;
                }
            } else {
                return false;
            }
        } else if (t.type == TOKEN_DELETE) {
            DeleteCommand c = {0};
            advance_token(&p->current);
            if (parse_delete(p, console_win, &c)) {
                if (execute_delete(c, console_win)) {
                    Command *cmd = ARENA_PUSH_OBJECT(a, Command);
                    cmd->type = DELETE;
                    cmd->cmds.delete_command = c;
                    add_ast_node(a, tree, cmd);
                    show_msg(console_win, "Entity/relationship deleted succesfully", "UPDATE");
                } else {
                    error_msg_ex(console_win, p, ERR_RUNTIME, "Could not delete",
                                 "no entity/relationship with that name");
                    break;
                }
            } else {
                return false;
            }
        } else if (t.type == TOKEN_CLEAR) {
            advance_token(&p->current);
            if (parse_clear(console_win, tree, a)) {
                // Clear succeeded — nothing to add to AST, but undo is already pushed
                show_msg(console_win, "Diagram cleared", "UPDATE");
            }
        } else if (t.type == TOKEN_SAVE) {
            SaveCommand c = {0};
            advance_token(&p->current);
            if (parse_save(p, console_win, &c)) {
                if (execute_save(c, console_win)) {
                    // Record in the AST so the debug window shows it
                    Command *cmd = ARENA_PUSH_OBJECT(a, Command);
                    cmd->type = SAVE;
                    cmd->cmds.save_command = c;
                    add_ast_node(a, tree, cmd);
                } else {
                    // execute_save / save_diagram already displayed the error
                    break;
                }
            } else {
                return false;
            }
        } else if (t.type == TOKEN_UNDO) {
            advance_token(&p->current);
            if (execute_undo(tree, a, console_win)) {
                // undo succeeded caller will set needs_redraw
            }
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
            const char *error =
                "Unknown command try : {create, add, card, convert, change, save, delete, undo, help, clear}";
            error_msg(console_win, p, error);
            return false;
        }
    }

    return true;
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
#define GRID_COLS 4
#define GRID_X_STEP (ENTITY_WIDTH + 4)
#define GRID_Y_STEP (ENTITY_HEIGHT + 3)
#define GRID_X_ORIGIN 2
#define GRID_Y_ORIGIN 2
    static int entity_count_placed = 0;

    if (c.type == TYPE_ENTITY) {
        if (!search_entity(c.Data.e.name)) {
            int col = entity_count_placed % GRID_COLS;
            int row = entity_count_placed / GRID_COLS;
            int x = GRID_X_ORIGIN + col * GRID_X_STEP;
            int y = GRID_Y_ORIGIN + row * GRID_Y_STEP;
            createEntity(c.Data.e.name, x, y);
            entity_count_placed++;
            return true;
        }
    } else {
        Entity *e1 = search_entity(c.Data.r.e1_name);
        Entity *e2 = search_entity(c.Data.r.e2_name);
        if (!e1) {
            int col = entity_count_placed % GRID_COLS;
            int row = entity_count_placed / GRID_COLS;
            e1 = createEntity(c.Data.r.e1_name, GRID_X_ORIGIN + col * GRID_X_STEP, GRID_Y_ORIGIN + row * GRID_Y_STEP);
            entity_count_placed++;
        }
        if (!e2) {
            int col = entity_count_placed % GRID_COLS;
            int row = entity_count_placed / GRID_COLS;
            e2 = createEntity(c.Data.r.e2_name, GRID_X_ORIGIN + col * GRID_X_STEP, GRID_Y_ORIGIN + row * GRID_Y_STEP);
            entity_count_placed++;
        }
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
        break;

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

static bool confirm_yes_no(WINDOW *console_win, const char *prompt) {
    wmove(console_win, 1, 1);
    wclrtoeol(console_win);
    wmove(console_win, 2, 1);
    wclrtoeol(console_win);
    wmove(console_win, 3, 1);
    wclrtoeol(console_win);

    wattron(console_win, COLOR_PAIR(3));
    mvwprintw(console_win, 1, 1, "[CONFIRM]: ");
    wattroff(console_win, COLOR_PAIR(3));
    mvwprintw(console_win, 1, 12, "%s", prompt);
    mvwprintw(console_win, 2, 1, "> (y/n): ");
    wrefresh(console_win);

    // Block for a single keypress, then restore non-blocking mode
    timeout(-1);
    int ch = getch();
    timeout(16);

    // Clear the prompt area
    wmove(console_win, 1, 1);
    wclrtoeol(console_win);
    wmove(console_win, 2, 1);
    wclrtoeol(console_win);
    wmove(console_win, 3, 1);
    wclrtoeol(console_win);
    wrefresh(console_win);

    return (ch == 'y' || ch == 'Y');
}

bool parse_clear(WINDOW *console_win, AST *tree, Arena *a) {
    if (!confirm_yes_no(console_win, "Clear entire diagram? This cannot be undone except with 'undo'.")) {
        show_msg(console_win, "Clear cancelled", "INFO");
        return false;
    }

    // Snapshot the current state BEFORE clearing, so undo can restore it
    UndoEntry ue = {0};
    ue.type = UNDO_CLEAR;
    ue.data.clear.snapshot = malloc(UNDO_SNAPSHOT_SIZE);
    if (ue.data.clear.snapshot) {
        ue.data.clear.snapshot_len = snapshot_diagram_to_buf(ue.data.clear.snapshot, UNDO_SNAPSHOT_SIZE);
    }

    // Now clear everything
    init_global_objects();

    // Also clear the AST since those commands no longer reflect reality
    init_AST(tree);

    undo_stack_push(&global_objects.undo_stack, ue);
    return true;
}

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

bool parse_delete(Parser *p, WINDOW *console, DeleteCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == TOKEN_STRING) {
        if (strlen(t.value) == 0) {
            const char *error = "Empty entity/relationship name";
            error_msg(console, p, error);
            return false;
        }
        strncpy(c->name, t.value, MAX_NAME_LEN);
        c->name[MAX_NAME_LEN - 1] = '\0';
        advance_token(&p->current);
        if (parse_delete_element(p, console, c)) {
            return true;
        }

    } else {
        const char *error = "Expected the entity/relationship name to delete";
        error_msg(console, p, error);
    }
    return false;
}

bool parse_delete_element(Parser *p, WINDOW *console, DeleteCommand *c) {
    Token t = peek_token(p->tokens, p->current);
    if (t.type == 0) {
        // No property provided (defaults to deleting the whole element Entity/Relationship)
        c->type = ELEMENT;
        return true;
    } else if (t.type == TOKEN_STRING) {
        // Delete a property

        if (strlen(t.value) == 0) {
            const char *error = "Empty property name";
            error_msg(console, p, error);
            return false;
        }

        c->type = PROP;
        strncpy(c->prop_name, t.value, MAX_NAME_LEN);
        advance_token(&p->current);
        return true;
    } else {
        const char *error = "Expected Either nothing (delete the entity/relationship) or a property name";
        error_msg(console, p, error);
    }
    return false;
}

bool execute_delete(DeleteCommand c, WINDOW *console_win) {
    Entity *e = search_entity(c.name);
    if (e) {
        if (c.type == ELEMENT) {
            UndoEntry ue = {0};
            ue.type = UNDO_DELETE;
            ue.data.delete.delete_type = 1;
            ue.data.delete.is_relationship = false;
            strncpy(ue.data.delete.name, e->name, MAX_NAME_LEN);
            ue.data.delete.x = e->x;
            ue.data.delete.y = e->y;
            ue.data.delete.num_properties = e->num_properties;
            for (int i = 0; i < e->num_properties; i++) {
                if (e->properties[i]) {
                    strncpy(ue.data.delete.props[i].name, e->properties[i]->name, MAX_NAME_LEN);
                    strncpy(ue.data.delete.props[i].type, e->properties[i]->type, MAX_TYPE_LEN);
                    ue.data.delete.props[i].keytype = (int)e->properties[i]->keytype;
                }
            }
            undo_stack_push(&global_objects.undo_stack, ue);

            int removed = 0;
            for (int i = global_objects.relationship_count - 1; i >= 0; i--) {
                Relationship *r = global_objects.relationships[i];
                if (r && (r->e1 == e || r->e2 == e)) {
                    unregister_relationship(r);
                    removed++;
                }
            }
            if (removed > 0) {
                char msg[128];
                snprintf(msg, sizeof(msg), "Also removed %d relationship(s) attached to \"%s\"", removed, e->name);
                show_msg(console_win, msg, "INFO");
            }
            unregister_entity(e);
            return true;
        } else if (c.type == PROP) {
            for (int i = 0; i < e->num_properties; i++) {
                if (e->properties[i] && mcd_strcasecmp(c.prop_name, e->properties[i]->name) == 0) {
                    UndoEntry ue = {0};
                    ue.type = UNDO_DELETE;
                    ue.data.delete.delete_type = 0;
                    ue.data.delete.is_relationship = false;
                    strncpy(ue.data.delete.name, e->name, MAX_NAME_LEN);
                    strncpy(ue.data.delete.prop_name, e->properties[i]->name, MAX_NAME_LEN);
                    strncpy(ue.data.delete.prop_type, e->properties[i]->type, MAX_TYPE_LEN);
                    ue.data.delete.keytype = (int)e->properties[i]->keytype;
                    undo_stack_push(&global_objects.undo_stack, ue);

                    free(e->properties[i]);
                    for (int j = i; j < e->num_properties - 1; j++) {
                        e->properties[j] = e->properties[j + 1];
                    }
                    e->properties[--e->num_properties] = NULL;
                    e->height -= 1;
                    return true;
                }
            }
            return false;
        }
    }

    Relationship *r = search_relationship(c.name);
    if (r) {
        if (c.type == ELEMENT) {
            UndoEntry ue = {0};
            ue.type = UNDO_DELETE;
            ue.data.delete.delete_type = 1;
            ue.data.delete.is_relationship = true;
            strncpy(ue.data.delete.name, r->name, MAX_NAME_LEN);
            ue.data.delete.x = r->x;
            ue.data.delete.y = r->y;
            ue.data.delete.num_properties = r->num_properties;
            for (int i = 0; i < r->num_properties; i++) {
                if (r->properties[i]) {
                    strncpy(ue.data.delete.props[i].name, r->properties[i]->name, MAX_NAME_LEN);
                    strncpy(ue.data.delete.props[i].type, r->properties[i]->type, MAX_TYPE_LEN);
                    ue.data.delete.props[i].keytype = (int)r->properties[i]->keytype;
                }
            }
            if (r->e1) {
                strncpy(ue.data.delete.e1_name, r->e1->name, MAX_NAME_LEN);
            }
            if (r->e2) {
                strncpy(ue.data.delete.e2_name, r->e2->name, MAX_NAME_LEN);
            }
            if (r->cards[0]) {
                ue.data.delete.had_card0 = true;
                strncpy(ue.data.delete.card0, r->cards[0]->value, CARDINALITY_LEN);
            }
            if (r->cards[1]) {
                ue.data.delete.had_card1 = true;
                strncpy(ue.data.delete.card1, r->cards[1]->value, CARDINALITY_LEN);
            }
            undo_stack_push(&global_objects.undo_stack, ue);

            unregister_relationship(r);
            return true;
        } else if (c.type == PROP) {
            for (int i = 0; i < r->num_properties; i++) {
                if (r->properties[i] && mcd_strcasecmp(c.prop_name, r->properties[i]->name) == 0) {
                    UndoEntry ue = {0};
                    ue.type = UNDO_DELETE;
                    ue.data.delete.delete_type = 0;
                    ue.data.delete.is_relationship = true;
                    strncpy(ue.data.delete.name, r->name, MAX_NAME_LEN);
                    strncpy(ue.data.delete.prop_name, r->properties[i]->name, MAX_NAME_LEN);
                    strncpy(ue.data.delete.prop_type, r->properties[i]->type, MAX_TYPE_LEN);
                    ue.data.delete.keytype = (int)r->properties[i]->keytype;
                    undo_stack_push(&global_objects.undo_stack, ue);

                    free(r->properties[i]);
                    for (int j = i; j < r->num_properties - 1; j++) {
                        r->properties[j] = r->properties[j + 1];
                    }
                    r->properties[--r->num_properties] = NULL;
                    r->height -= 1;
                    return true;
                }
            }
            return false;
        }
    }

    return false;
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
        const char *error =
            "Expected a valid conversion mode (MLD) if you wish to convert to SQL use 'save SQL' command instead";
        error_msg(win, p, error);
        return false;
    }
}

static void build_unique_property_name(Entity *dst, const char *prop_name, const char *src_name, char *out,
                                       size_t out_size) {
    bool clash = false;
    for (int i = 0; i < dst->num_properties; i++) {
        if (dst->properties[i] && strcmp(dst->properties[i]->name, prop_name) == 0) {
            clash = true;
            break;
        }
    }

    if (!clash) {
        strncpy(out, prop_name, out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }

    snprintf(out, out_size, "%s_%s", src_name, prop_name);
}

static void migrate_foreign_key(Entity *dst, Entity *src, Relationship *r) {
    for (int i = 0; i < src->num_properties; i++) {
        if (src->properties[i] && src->properties[i]->keytype == PRIMARY_KEY) {
            char name[MAX_NAME_LEN];
            // For self-referencing relationships (dst == src), use the relationship
            // name to build a descriptive FK column (e.g. "SubCat_cat_id").
            // Otherwise use the standard "SrcName_prop_name" pattern.
            if (dst == src) {
                char prefix[MAX_NAME_LEN];
                char tmp[MAX_NAME_LEN * 2];
                snprintf(tmp, sizeof(tmp), "%s_%s", r->name, src->properties[i]->name);
                strncpy(prefix, tmp, MAX_NAME_LEN - 1);
                prefix[MAX_NAME_LEN - 1] = '\0';
                build_unique_property_name(dst, prefix, src->name, name, sizeof(name));
            } else {
                build_unique_property_name(dst, src->properties[i]->name, src->name, name, sizeof(name));
            }
            addProperty(dst, name, src->properties[i]->type, FOREIGN_KEY);
            // references stores the SOURCE ENTITY name (for SQL REFERENCES clause)
            set_property_reference(dst, name, src->name);
            // references_column stores the ORIGINAL PK column name in the source entity
            set_property_reference_column(dst, name, src->properties[i]->name);
        }
    }
    for (int i = 0; i < r->num_properties; i++) {
        if (r->properties[i]) {
            char name[MAX_NAME_LEN];
            build_unique_property_name(dst, r->properties[i]->name, r->name, name, sizeof(name));
            addProperty(dst, name, r->properties[i]->type, r->properties[i]->keytype);
        }
    }
}

static bool create_junction_entity(Relationship *r, Entity *e1, Entity *e2) {
    char junction_name[MAX_NAME_LEN];
    char jname_tmp[MAX_NAME_LEN * 2];
    snprintf(jname_tmp, sizeof(jname_tmp), "%s_%s", e1->name, e2->name);
    strncpy(junction_name, jname_tmp, MAX_NAME_LEN - 1);
    junction_name[MAX_NAME_LEN - 1] = '\0';

    if (search_entity(junction_name) || search_relationship(junction_name)) {
        return false;
    }

    Entity *junction = createEntity(junction_name, r->x, r->y);
    if (!junction) {
        return false;
    }

    // Migrate PKs from e1 and e2.  Each column is PRIMARY_KEY in the
    // junction table (so it participates in the composite PK) and also has
    // its 'references' field set (so generate_sql() emits the FK).
    // This avoids duplicating columns as separate PK + FK properties.
    for (int i = 0; i < e1->num_properties; i++) {
        if (e1->properties[i] && e1->properties[i]->keytype == PRIMARY_KEY) {
            char name[MAX_NAME_LEN];
            build_unique_property_name(junction, e1->properties[i]->name, e1->name, name, sizeof(name));
            addProperty(junction, name, e1->properties[i]->type, PRIMARY_KEY);
            set_property_reference(junction, name, e1->name);
        }
    }
    for (int i = 0; i < e2->num_properties; i++) {
        if (e2->properties[i] && e2->properties[i]->keytype == PRIMARY_KEY) {
            char name[MAX_NAME_LEN];
            build_unique_property_name(junction, e2->properties[i]->name, e2->name, name, sizeof(name));
            addProperty(junction, name, e2->properties[i]->type, PRIMARY_KEY);
            set_property_reference(junction, name, e2->name);
        }
    }

    // Migrate any relationship properties (e.g. "qty" on an N:N relationship)
    for (int i = 0; i < r->num_properties; i++) {
        if (r->properties[i]) {
            char name[MAX_NAME_LEN];
            build_unique_property_name(junction, r->properties[i]->name, r->name, name, sizeof(name));
            addProperty(junction, name, r->properties[i]->type, r->properties[i]->keytype);
        }
    }
    return true;
}

bool convert_to_mld(WINDOW *console_win) {
    bool had_skipped = false;

    // traverse backwards because relationships are getting
    // shifted when deleted which means we are missing some
    // check global_objects.c file to understand the
    // unregister relationship implementation
    for (int i = global_objects.relationship_count - 1; i >= 0; i--) {
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
        if (!isalnum(name[i]) && name[i] != '_') {
            return false;
        }
    }

    return true;
}

bool parse_save(Parser *p, WINDOW *console, SaveCommand *c) {
    Token t = peek_token(p->tokens, p->current);

    switch (t.type) {
    case TOKEN_MCD:
        c->diagram_type = MCD;
        advance_token(&p->current);
        break;
    case TOKEN_MLD:
        c->diagram_type = MLD;
        advance_token(&p->current);
        break;
    case TOKEN_SQL:
        if (global_objects.current_dtype != MLD) {
            error_msg(console, p, "Cannot convert to sql without converting to MLD first");
            return false;
        }
        c->diagram_type = SQL;
        advance_token(&p->current);
        break;
    default:
        error_msg(console, p, "Expected diagram type after 'save' (MCD or MLD or SQL)");
        return false;
    }

    t = peek_token(p->tokens, p->current);
    if (t.type != TOKEN_STRING) {
        error_msg(console, p,
                  "Expected filename after diagram type (e.g. for MCD/MLD: \"mydiagram.txt\" for SQL: \"schema.sql\")");
        return false;
    }
    if (t.value[0] == '\0') {
        error_msg(console, p, "Filename cannot be empty");
        return false;
    }

    strncpy(c->filename, t.value, MAX_FILENAME_LEN - 1);
    c->filename[MAX_FILENAME_LEN - 1] = '\0';
    advance_token(&p->current);
    return true;
}

// execute_save: thin wrapper around save_diagram() from utils/save.c.
bool execute_save(SaveCommand c, WINDOW *console_win) {
    if (c.diagram_type == SQL) {
        return generate_sql(c.filename, console_win);
    }

    return save_diagram(c.filename, c.diagram_type, console_win);
}

// For MLD undo: write snapshot to a tmpfile and load it back.
// tmpfile() is standard C89 and works on both Windows and Linux.
static bool save_snapshot_to_tmpfile(char *buf, size_t len, FILE **out_fp) {
    FILE *fp = tmpfile();
    if (!fp)
        return false;
    if (fwrite(buf, 1, len, fp) != len) {
        fclose(fp);
        return false;
    }
    rewind(fp);
    *out_fp = fp;
    return true;
}

// Restores the diagram from a snapshot buffer produced by snapshot_diagram_to_buf.
// Uses a tmpfile so load_diagram can slurp it back in.  Clears global_objects first.
static void restore_from_snapshot(const char *buf, size_t len, AST *tree, Arena *a, WINDOW *console_win) {
    init_global_objects();

    FILE *fp = NULL;
    if (!save_snapshot_to_tmpfile((char *)buf, len, &fp)) {
        show_msg(console_win, "Undo MLD failed: could not create temp file", "WARNING");
        return;
    }

    // Build a temp filename from the FILE* — load_diagram needs a path.
    // tmpfile() gives us a FILE* but no name.  Instead, use a named temp
    // file with mkstemp / tmpnam.  Actually, simpler: write to a fixed
    // temp name in current dir, load it, then delete it.
    fclose(fp);

    // Simpler approach: just write to ".mcd_undo_tmp.txt", load, then remove.
    const char *tmpname = ".mcd_undo_tmp.txt";
    FILE *w = fopen(tmpname, "w");
    if (!w) {
        show_msg(console_win, "Undo MLD failed: could not write temp file", "WARNING");
        return;
    }
    fwrite(buf, 1, len, w);
    fclose(w);

    bool dummy = false;
    load_diagram(tmpname, a, tree, console_win, &dummy);
    remove(tmpname);

    // Prevent the restored commands from being individually undoable
    undo_stack_clear(&global_objects.undo_stack);
}

bool execute_undo(AST *tree, Arena *a, WINDOW *console_win) {
    UndoEntry entry;
    if (!undo_stack_pop(&global_objects.undo_stack, &entry)) {
        show_msg(console_win, "Nothing to undo", "INFO");
        return false;
    }

    switch (entry.type) {

    case UNDO_CREATE_ENTITY: {
        Entity *e = search_entity(entry.data.create_entity.name);
        if (!e)
            return false;
        // also clean up any relationships that were created after this entity
        // and happen to still reference it
        for (int i = global_objects.relationship_count - 1; i >= 0; i--) {
            Relationship *r = global_objects.relationships[i];
            if (r && (r->e1 == e || r->e2 == e)) {
                unregister_relationship(r);
            }
        }
        unregister_entity(e);
        show_msg(console_win, "Undid: create entity", "UPDATE");
        return true;
    }

    case UNDO_CREATE_RELATIONSHIP: {
        Relationship *r = search_relationship(entry.data.create_rel.name);
        if (!r)
            return false;
        unregister_relationship(r);
        show_msg(console_win, "Undid: create relationship", "UPDATE");
        return true;
    }

    case UNDO_ADD_PROP: {
        const char *id = entry.data.add_prop.identifier_name;
        const char *pname = entry.data.add_prop.prop_name;
        if (entry.data.add_prop.is_relationship) {
            Relationship *r = search_relationship(id);
            if (!r)
                return false;
            for (int i = 0; i < r->num_properties; i++) {
                if (r->properties[i] && mcd_strcasecmp(r->properties[i]->name, pname) == 0) {
                    free(r->properties[i]);
                    for (int j = i; j < r->num_properties - 1; j++)
                        r->properties[j] = r->properties[j + 1];
                    r->properties[--r->num_properties] = NULL;
                    r->height -= 1;
                    break;
                }
            }
        } else {
            Entity *e = search_entity(id);
            if (!e)
                return false;
            for (int i = 0; i < e->num_properties; i++) {
                if (e->properties[i] && mcd_strcasecmp(e->properties[i]->name, pname) == 0) {
                    free(e->properties[i]);
                    for (int j = i; j < e->num_properties - 1; j++)
                        e->properties[j] = e->properties[j + 1];
                    e->properties[--e->num_properties] = NULL;
                    e->height -= 1;
                    break;
                }
            }
        }
        show_msg(console_win, "Undid: add property", "UPDATE");
        return true;
    }

    case UNDO_ADD_CARD: {
        Relationship *r = search_relationship(entry.data.add_card.rel_name);
        if (!r)
            return false;
        if (entry.data.add_card.had_card0) {
            Cardinality *c = malloc(sizeof(Cardinality));
            if (c) {
                strncpy(c->value, entry.data.add_card.card0, CARDINALITY_LEN - 1);
                c->value[CARDINALITY_LEN - 1] = '\0';
                if (r->cards[0])
                    free(r->cards[0]);
                r->cards[0] = c;
            }
        } else {
            if (r->cards[0]) {
                free(r->cards[0]);
                r->cards[0] = NULL;
            }
        }
        if (entry.data.add_card.had_card1) {
            Cardinality *c = malloc(sizeof(Cardinality));
            if (c) {
                strncpy(c->value, entry.data.add_card.card1, CARDINALITY_LEN - 1);
                c->value[CARDINALITY_LEN - 1] = '\0';
                if (r->cards[1])
                    free(r->cards[1]);
                r->cards[1] = c;
            }
        } else {
            if (r->cards[1]) {
                free(r->cards[1]);
                r->cards[1] = NULL;
            }
        }
        show_msg(console_win, "Undid: add card", "UPDATE");
        return true;
    }

    case UNDO_CHANGE_NAME: {
        // The inverse of "change name A -> B" is "change name B -> A"
        Entity *e = search_entity(entry.data.change_name.new_name);
        if (e) {
            strncpy(e->name, entry.data.change_name.old_name, MAX_NAME_LEN);
            e->name[MAX_NAME_LEN - 1] = '\0';
            show_msg(console_win, "Undid: change name", "UPDATE");
            return true;
        }
        Relationship *r = search_relationship(entry.data.change_name.new_name);
        if (r) {
            strncpy(r->name, entry.data.change_name.old_name, MAX_NAME_LEN);
            r->name[MAX_NAME_LEN - 1] = '\0';
            show_msg(console_win, "Undid: change name", "UPDATE");
            return true;
        }
        return false;
    }

    case UNDO_CONVERT_MLD: {
        // Replay the pre-conversion snapshot to restore the MCD state
        restore_from_snapshot(entry.data.convert_mld.snapshot, entry.data.convert_mld.snapshot_len, tree, a, console_win);
        undo_entry_free(&entry);
        show_msg(console_win, "Undid: convert MLD", "UPDATE");
        return true;
    }

    case UNDO_DELETE: {
        if (entry.data.delete.delete_type == 1) {
            // Restore an entire deleted element (entity or relationship)
            if (!entry.data.delete.is_relationship) {
                // --- Re-create the entity ---
                Entity *e = createEntity(entry.data.delete.name, entry.data.delete.x, entry.data.delete.y);
                if (!e)
                    return false;

                // Replay all stored properties
                for (int i = 0; i < entry.data.delete.num_properties; i++) {
                    addProperty(e, entry.data.delete.props[i].name, entry.data.delete.props[i].type,
                                (KeyType)entry.data.delete.props[i].keytype);
                }
                show_msg(console_win, "Undid: delete entity", "UPDATE");
                return true;
            } else {
                // --- Re-create the relationship ---
                Entity *e1 = search_entity(entry.data.delete.e1_name);
                Entity *e2 = search_entity(entry.data.delete.e2_name);
                if (!e1 || !e2) {
                    show_msg(console_win, "Undo delete failed: attached entities no longer exist", "WARNING");
                    return false;
                }

                Relationship *r =
                    addRelationship(entry.data.delete.x, entry.data.delete.y, e1, e2, entry.data.delete.name);
                if (!r)
                    return false;

                // Replay properties
                for (int i = 0; i < entry.data.delete.num_properties; i++) {
                    addPropertyRelationship(r, entry.data.delete.props[i].name, entry.data.delete.props[i].type,
                                            (KeyType)entry.data.delete.props[i].keytype);
                }

                // Replay cardinalities if they existed
                if (entry.data.delete.had_card0) {
                    Cardinality *c = malloc(sizeof(Cardinality));
                    if (c) {
                        strncpy(c->value, entry.data.delete.card0, CARDINALITY_LEN - 1);
                        c->value[CARDINALITY_LEN - 1] = '\0';
                        r->cards[0] = c;
                    }
                }
                if (entry.data.delete.had_card1) {
                    Cardinality *c = malloc(sizeof(Cardinality));
                    if (c) {
                        strncpy(c->value, entry.data.delete.card1, CARDINALITY_LEN - 1);
                        c->value[CARDINALITY_LEN - 1] = '\0';
                        r->cards[1] = c;
                    }
                }
                show_msg(console_win, "Undid: delete relationship", "UPDATE");
                return true;
            }
        } else {
            // --- Restore a deleted property ---
            if (!entry.data.delete.is_relationship) {
                Entity *e = search_entity(entry.data.delete.name);
                if (!e)
                    return false;
                addProperty(e, entry.data.delete.prop_name, entry.data.delete.prop_type,
                            (KeyType)entry.data.delete.keytype);
                show_msg(console_win, "Undid: delete property", "UPDATE");
                return true;
            } else {
                Relationship *r = search_relationship(entry.data.delete.name);
                if (!r)
                    return false;
                addPropertyRelationship(r, entry.data.delete.prop_name, entry.data.delete.prop_type,
                                        (KeyType)entry.data.delete.keytype);
                show_msg(console_win, "Undid: delete property", "UPDATE");
                return true;
            }
        }
    }
    case UNDO_CLEAR: {
        restore_from_snapshot(entry.data.clear.snapshot, entry.data.clear.snapshot_len, tree, a, console_win);
        undo_entry_free(&entry);
        show_msg(console_win, "Undid: clear diagram", "UPDATE");
        return true;
    }

    default:
        return false;
    }

    return false;
}
