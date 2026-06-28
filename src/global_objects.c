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

    undo_stack_init(&global_objects.undo_stack);
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

void unregister_relationship(Relationship *r) {
    if (!r) {
        return;
    }
    for (int i = 0; i < global_objects.relationship_count; i++) {
        if (global_objects.relationships[i] == r) {
            for (int j = 0; j < r->num_properties; j++) {
                if (r->properties[j]) {
                    free(r->properties[j]);
                }
            }
            if (r->cards[0]) {
                free(r->cards[0]);
            }
            // BUG FIX: cards[1] was never freed, causing a memory leak
            if (r->cards[1]) {
                free(r->cards[1]);
            }
            free(r);
            // Shift remaining elements down to fill the hole
            for (int j = i; j < global_objects.relationship_count - 1; j++) {
                global_objects.relationships[j] = global_objects.relationships[j + 1];
            }
            global_objects.relationships[--global_objects.relationship_count] = NULL;
            return;
        }
    }
}

void unregister_entity(Entity *e) {
    if (!e) {
        return;
    }

    for (int i = 0; i < global_objects.entity_count; i++) {
        Entity *curr = global_objects.entities[i];
        if (curr == e) {
            for (int j = 0; j < e->num_properties; j++) {
                // BUG FIX: was e->properties[i] (outer loop index) instead of
                // e->properties[j], causing the wrong slots to be freed and
                // triggering a double-free when unregister_entity was called
                // after execute_delete had already nulled some of them
                if (e->properties[j]) {
                    free(e->properties[j]);
                }
            }

            free(e);
            // Shift remaining elements down to fill the hole
            for (int j = i; j < global_objects.entity_count - 1; j++) {
                global_objects.entities[j] = global_objects.entities[j + 1];
            }
            global_objects.entities[--global_objects.entity_count] = NULL;
            return;
        }
    }
}
