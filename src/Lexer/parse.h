#pragma once
#include "../DSA/AST.h" // includes arena.h
#include "tokenize.h"
#include <ncurses.h>

typedef struct {
        Token tokens[MAX_TOKENS_NUM_PER_COMMAND];
        int count;
        int current;
        const char *userInput;
} Parser;

void init_parser(Parser *p, const char *content);
bool parse_command(AST *tree, Parser *p, const char *content, WINDOW *console_win, Arena *a);
// maybe in future implementation of error msg function we can add a simple
// guess 'did you mean this "TOKEN"' then we have to pass the current token
void error_msg(WINDOW *console_win, Parser *p, const char *error);

typedef enum { ERR_SYNTAX, ERR_RUNTIME } ErrorKind;
void error_msg_ex(WINDOW *console_win, Parser *p, ErrorKind kind, const char *error, const char *hint);

bool parse_create(Parser *p, WINDOW *console, CreateCommand *c);
bool parse_create_entity(WINDOW *console, Parser *p, bool is_called, CreateCommand *c);
bool parse_create_relationship(WINDOW *console, Parser *p, CreateCommand *c);

// to be moved to a seperate file for execution
bool execute_create(CreateCommand c);

// Add property command
bool parse_add(Parser *p, WINDOW *console, AddCommand *c);
bool parse_add_property(Parser *p, WINDOW *console, AddCommand *c);
bool parse_add_property_name(Parser *p, WINDOW *console, AddCommand *c);
bool parse_add_property_type(Parser *p, WINDOW *console, AddCommand *c);
Element *get_element_by_name(const char *name);
bool parse_add_property_key(Parser *p, WINDOW *console, AddCommand *c);
bool execute_addProperty(AddCommand c, WINDOW *console_win);

//  Add cardinality command
bool parse_add_cardinality(Parser *p, WINDOW *console, AddCommand *c);
bool parse_add_cardinality_value(Parser *p, WINDOW *console, AddCommand *c);
bool execute_addCardinality(AddCommand c, WINDOW *console_win);

// Convert to MLD command
bool parse_convert(Parser *p, WINDOW *win, ConvertCommand *c);
bool convert_to_mld(WINDOW *console_win);

// Change name command
bool parse_change_name(Parser *p, WINDOW *console, ChangeNameCommand *c);
bool parse_change_name_value(Parser *p, WINDOW *console, ChangeNameCommand *c);
bool execute_changeName(ChangeNameCommand c, WINDOW *console_win);

// clear command
void parse_clear();

// display commands
void show_msg(WINDOW *console_win, const char *msg, const char *severity);

bool valid_name(const char *name);
