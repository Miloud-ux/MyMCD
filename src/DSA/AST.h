#pragma once

#include "../MCD_elements.h"
#include "../utils/arena_allocator.h"

/* == ASTNodes Data types == */
typedef struct {
        char name[MAX_NAME_LEN];
} EntityInfo;

typedef struct {
        char name[MAX_NAME_LEN];
        char e1_name[MAX_NAME_LEN];
        char e2_name[MAX_NAME_LEN];
} RelationshipInfo;

typedef struct {
        char prop_name[MAX_NAME_LEN];
        char prop_type[MAX_TYPE_LEN];
        KeyType type;
} PropertyInfo;

typedef struct {
        char value[RAW_CARDINALITY_LEN];
        char entity_name[MAX_NAME_LEN];
        RelationshipInfo r;
} CardinalityInfo;

typedef enum { TYPE_ENTITY, TYPE_RELATIONSHIP, TYPE_PROPERTY, TYPE_CARDINALITY } ElementType;

typedef struct {
        ElementType type;
        union {
                Entity *e;
                Relationship *r;
                Property *p;
                Cardinality *c;
        } Element;
} Element;

typedef struct {
        ElementType type;
        union {
                EntityInfo e;
                RelationshipInfo r;
        } Data;
} CreateCommand;

typedef struct {
        char identifier_name[MAX_NAME_LEN];
        ElementType type;
        union {
                CardinalityInfo c;
                PropertyInfo p;
        } Data;
} AddCommand;

typedef enum { ADD, CREATE, CONVERT, CHANGE_NAME } CommandType;
typedef enum { MCD, MLD, SQL } DiagramType;

typedef struct {
        DiagramType type;
} ConvertCommand;

typedef struct {
        char old_name[MAX_NAME_LEN];
        char new_name[MAX_NAME_LEN];
} ChangeNameCommand;

typedef union {
        AddCommand add_command;
        CreateCommand create_command;
        ConvertCommand convert_command;
        ChangeNameCommand change_name_command;
} CommandsContainer;

typedef struct {
        CommandType type;
        CommandsContainer cmds;
} Command;

// AST Structure
typedef enum { LOW, MEDIUM, HIGH } Priority; // for nested commands
typedef struct ASTNode {
        Command *cmd;
        Priority p;
        struct ASTNode *next;
} ASTNode;

typedef struct AST {
        ASTNode *head;
        ASTNode *tail;
        int ASTNode_num;
} AST;

void init_AST(AST *t);
ASTNode *create_ast_node(Arena *a, Command *cmd, Priority p); // instead of passing a copy we pass a refernce
bool add_ast_node(Arena *a, AST *t, Command *cmd);
bool delete_ast_node(AST *t);
