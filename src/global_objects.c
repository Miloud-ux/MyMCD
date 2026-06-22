// == CHANGES : ==
// - Added unregister_relationship() after register_relationship(). It finds
//   the relationship by pointer in global_objects.relationships[], frees its
//   properties and its cardinality pair (cards[0]/cards[1] are the same
//   malloc'd block from addCardinalityAPI, so only cards[0] is freed), frees
//   the relationship itself, then sets the array slot to NULL (matching how
//   draw_all_relationships/get_attach_slot etc. already guard against NULL
//   entries instead of compacting the array).
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
            free(r);
            global_objects.relationships[i] = NULL;
            return;
        }
    }
}
