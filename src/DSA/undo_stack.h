#pragma once

#include "AST.h"
#include <stdbool.h>
#include <stddef.h>

#define UNDO_STACK_CAPACITY 64

// 64 KB snapshot buffer enough for a regular diagram serialised
#define UNDO_SNAPSHOT_SIZE (64 * 1024)

typedef UndoType UndoEntryType;

typedef struct {
        char name[15];
} UndoCreateEntity;

typedef struct {
        char name[15];
} UndoCreateRelationship;

typedef struct {
        char identifier_name[15];
        char prop_name[15];
        char prop_type[7];
        int keytype;
        bool is_relationship;
} UndoAddProp;

typedef struct {
        char rel_name[15];
        char card0[4]; // previous cards[0]->value, "" if none
        char card1[4];
        bool had_card0;
        bool had_card1;
} UndoAddCard;

typedef struct {
        char old_name[15];
        char new_name[15];
} UndoChangeName;

// full diagram snapshot stored as plain text commands (same format as
// save_diagram).  The snapshot is heap-allocated so the entry stays small;
// the caller that pushes it transfers ownership, and undo_stack frees it on
// drop/evict/init.
typedef struct {
        char *snapshot;
        size_t snapshot_len;
} UndoConvertMld;

typedef struct {
        char *snapshot;
        size_t snapshot_len;
} UndoClear;

typedef struct {
        int delete_type; // 0 = property, 1 = element
        char name[MAX_NAME_LEN];
        char prop_name[MAX_NAME_LEN];
        char prop_type[MAX_TYPE_LEN];
        int keytype;
        bool is_relationship;
        int x, y;
        int num_properties;
        struct {
                char name[MAX_NAME_LEN];
                char type[MAX_TYPE_LEN];
                int keytype;
        } props[MAX_PROPERTIES];
        char e1_name[MAX_NAME_LEN];
        char e2_name[MAX_NAME_LEN];
        char card0[CARDINALITY_LEN];
        char card1[CARDINALITY_LEN];
        bool had_card0, had_card1;
} UndoDelete;

typedef struct {
        UndoEntryType type;
        union {
                UndoCreateEntity create_entity;
                UndoCreateRelationship create_rel;
                UndoAddProp add_prop;
                UndoAddCard add_card;
                UndoChangeName change_name;
                UndoConvertMld convert_mld;
                UndoDelete delete;
                UndoClear clear;
        } data;
} UndoEntry;

typedef struct {
        UndoEntry entries[UNDO_STACK_CAPACITY];
        int top; // -1 means empty
} UndoStack;

void undo_stack_init(UndoStack *s);
void undo_stack_clear(UndoStack *s);
bool undo_stack_push(UndoStack *s, UndoEntry entry);
bool undo_stack_pop(UndoStack *s, UndoEntry *out);
bool undo_stack_is_empty(const UndoStack *s);
void undo_entry_free(UndoEntry *e);
