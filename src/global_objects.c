#include "global_objects.h"
#include <stdlib.h>

GlobalObjects global_objects;

void init_global_objects() {
    global_objects.entity_count = 0;
    global_objects.relationship_count = 0;
    global_objects.current_dtype = MCD;

    // TODO: verify if this is causing a memory leak
    for (int i = 0; i < MAX_OBJECTS; i++) {
        global_objects.entities[i] = NULL;
        global_objects.relationships[i] = NULL;
    }
}

void register_entity(Entity *e) {
    if (global_objects.entity_count < MAX_OBJECTS) {
        global_objects.entities[global_objects.entity_count++] = e;
    }
}

void register_relationship(Relationship *r) {
    if (global_objects.relationship_count < MAX_OBJECTS) {
        global_objects.relationships[global_objects.relationship_count++] = r;
    }
}
