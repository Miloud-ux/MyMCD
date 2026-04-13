#include "command_processor.h"
#include "Lexer/parse.h"
#include "Lexer/tokenize.h"
#include "MCD_elements.h"
#include "global_objects.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static void to_lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

void show_message(WINDOW *console_win, const char *msg) {
    wmove(console_win, 1, 1);
    wclrtoeol(console_win);
    mvwprintw(console_win, 1, 1, ">> %s", msg);
    // not needed but keep just in case
    // box(console_win, 0, 0);
    // mvwprintw(console_win, 0, 2, " Console ");
    wrefresh(console_win);
}

void da_execute(WINDOW *console_win, const char *input, bool *needs_redraw) {
    Parser *p = malloc(sizeof(Parser));
    assert(p != NULL);
    init_parser(p, input);
    tokenize_content(input, p->tokens, &p->count);
    parse_command(p, input, console_win);
    *needs_redraw = true;
    free(p);
}

void execute_command(WINDOW *console_win, const char *input, bool *needs_redraw) {
    if (strlen(input) == 0) {
        return;
    }

    char buffer[256];
    strncpy(buffer, input, 255);
    buffer[255] = '\0';
    to_lowercase(buffer);

    char *words[10];
    int word_count = 0;

    char *token = strtok(buffer, " ");
    while (token != NULL && word_count < 10) {
        words[word_count++] = token;
        token = strtok(NULL, " ");
    }

    if (word_count == 0) {
        return;
    }

    if (strcmp(words[0], "create") == 0) {
        if (word_count >= 3 && strcmp(words[1], "entity") == 0) {
            char *name = words[2];

            for (int i = 0; i < global_objects.entity_count; i++) {
                if (strcasecmp(global_objects.entities[i]->name, name) == 0) {
                    show_message(console_win, "Error: Entity already exists!");
                    return;
                }
            }

            int x = 1 + (global_objects.entity_count) * 30;
            int y = 2 + (global_objects.entity_count / 4) * 12;

            Entity *new_entity = createEntity(name, x, y);

            char msg[128];
            snprintf(msg, sizeof(msg), "Created entity '%s' at (%d,%d)", name, x, y);
            show_message(console_win, msg);

            *needs_redraw = true;
        } else if (word_count >= 5 && strcmp(words[1], "relationship") == 0) {
            char *name = words[2];
            char *first_entity = words[3];
            char *second_entity = words[4];

            for (int i = 0; i < global_objects.relationship_count; i++) {
                if (strcasecmp(global_objects.relationships[i]->name, name) == 0) {
                    show_message(console_win, "Error: Relationship already exists!");
                    return;
                }
            }

            Entity *e1 = search_entity(first_entity);
            Entity *e2 = search_entity(second_entity);

            if (!e1 || !e2) {
                show_message(console_win, "Error: One or both entities not found");
                return;
            }

            int x = 1 + (global_objects.relationship_count % 4) * 10;
            int y = 5 + (global_objects.relationship_count / 4) * 12;

            Relationship *r = addRelationship(x, y, e1, e2, name);

            char msg[128];
            snprintf(msg, sizeof(msg), "Created relationship '%s'", name);
            show_message(console_win, msg);

            *needs_redraw = true;
        } else {
            show_message(console_win, "Usage: create entity <name> OR create relationship "
                                      "<name> <entity1> <entity2>");
        }
    } else if (strcmp(words[0], "help") == 0) {
        show_message(console_win, "Commands: create entity <name>, create "
                                  "relationship <name> <e1> <e2>, clear, quit");
    } else if (strcmp(words[0], "clear") == 0) {
        for (int i = 0; i < global_objects.entity_count; i++) {
            free(global_objects.entities[i]);
            global_objects.entities[i] = NULL;
        }
        global_objects.entity_count = 0;

        for (int i = 0; i < global_objects.relationship_count; i++) {
            free(global_objects.relationships[i]);
            global_objects.relationships[i] = NULL;
        }
        global_objects.relationship_count = 0;

        show_message(console_win, "Cleared all");
        *needs_redraw = true;
    } else if (strcmp(words[0], "add") == 0) {
        if (strcmp(words[1], "property") == 0 && word_count >= 5) {
            Entity *e = search_entity(words[4]);
            Relationship *r = search_relationship(words[4]);

            if (e != NULL) {
                addProperty(e, words[2], words[3]);
                *needs_redraw = true;
            } else if (r != NULL) {
                addPropertyRelationship(r, words[2], words[3]);
                *needs_redraw = true;
            } else {
                show_message(console_win, "Property Or relationship doesn't exist");
                return;
            }
        } else {
            show_message(console_win, "Usage: add property <pname> <ptype> <entity>/<relationship");
        }
    } else {
        char msg[128];
        snprintf(msg, sizeof(msg), "Unknown command: %s (type 'help')", words[0]);
        show_message(console_win, msg);
    }
}
