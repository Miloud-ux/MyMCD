#ifndef SRC_PARSE_H
#define SRC_PARSE_H
#include "../MCD_elements.h"
#include "tokenize.h"
#include <ncurses.h>

typedef struct {
        Token tokens[64];
        int count;
        int current;
        const char *userInput;
} Parser;

typedef struct {
        char name[MAX_NAME_LEN];
} EntityInfo;

typedef struct {
        char name[MAX_NAME_LEN];
        char e1_name[MAX_NAME_LEN];
        char e2_name[MAX_NAME_LEN];
} RelationshipInfo;

typedef enum { TYPE_ENTITY, TYPE_RELATIONSHIP } CreateType;

typedef struct {
        CreateType type;
        union {
                EntityInfo e;
                RelationshipInfo r;
        } Data;
} CreateCommand;

void init_parser(Parser *p, const char *content);
void parse_command(Parser *p, const char *content, WINDOW *console_win);
// maybe in future implementation of error msg function we can add a simple
// guess 'did you mean this "TOKEN"' then we have to pass the current token
void error_msg(WINDOW *console_win, Parser *p, const char *error);
bool parse_create(Parser *p, WINDOW *console, CreateCommand *c);
bool parse_create_entity(WINDOW *console, Parser *p, bool is_called, CreateCommand *c);
bool parse_create_relationship(WINDOW *console, Parser *p, CreateCommand *c);

// to be moved to a seperate file for execution
void execute_create(CreateCommand c);

#endif // !SRC_PARSE_H
