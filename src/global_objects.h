#ifndef GLOBAL_OBJECTS_H
#define GLOBAL_OBJECTS_H

#include "./DSA/AST.h"
#include "MCD_elements.h"

#define MAX_OBJECTS 100

typedef struct {
        Entity *entities[MAX_OBJECTS];
        Relationship *relationships[MAX_OBJECTS];
        int entity_count;
        int relationship_count;
        DiagramType current_dtype;
} GlobalObjects;

extern GlobalObjects global_objects;

void init_global_objects();
void register_entity(Entity *e);
void register_relationship(Relationship *r);

#endif
