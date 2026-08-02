#pragma once
#include "./DSA/AST.h"
#include "./DSA/undo_stack.h"
#include "MCD_elements.h"

#define MAX_OBJECTS 100

typedef struct {
        Entity *entities[MAX_OBJECTS];
        Relationship *relationships[MAX_OBJECTS];
        int entity_count;
        int relationship_count;
        DiagramType current_dtype;
        UndoStack undo_stack;
} GlobalObjects;

extern GlobalObjects global_objects;

// Bumped whenever an entity/relationship is registered or unregistered, so
// frame-based caches (route grids/paths) can detect structural changes.
extern int world_generation;

void init_global_objects();
void register_entity(Entity *e);
void unregister_entity(Entity *e);
void register_relationship(Relationship *r);
void unregister_relationship(Relationship *r);
