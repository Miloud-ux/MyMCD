#include "MCD_elements.h"
#include "global_objects.h"
#include <stdlib.h>
#include <string.h>

// Entity *createEntity(const char *name, int x, int y) {
//     int a = abs(3);
//     Entity *e = malloc(sizeof(Entity));
//     if (!e) {
//         return NULL;
//     }
//
//     strncpy(e->name, name, MAX_NAME_LEN - 1);
//     e->name[MAX_NAME_LEN - 1] = '\0';
//
//     e->x = x;
//     e->y = y;
//     e->height = ENTITY_HEIGHT;
//     e->width = ENTITY_WIDTH;
//     e->num_properties = 0;
//
//     for (int i = 0; i < MAX_PROPERTIES; i++) {
//         e->properties[i] = NULL;
//     }
//
//     // Alert user that creation done
//
//     return e;
// }

Entity *createEntity(const char *name, int x, int y) {
    Entity *e = malloc(sizeof(Entity));
    if (!e)
        return NULL;

    strncpy(e->name, name, MAX_NAME_LEN - 1);
    e->name[MAX_NAME_LEN - 1] = '\0';

    e->x = x;
    e->y = y;
    e->height = ENTITY_HEIGHT;
    e->width = ENTITY_WIDTH;
    e->num_properties = 0;

    for (int i = 0; i < MAX_PROPERTIES; i++) {
        e->properties[i] = NULL;
    }

    register_entity(e);
    return e;
}

void addProperty(Entity *e, const char *prop_name, const char *prop_type) {
    if (e->num_properties >= MAX_PROPERTIES) {
        return;
    }

    Property *p1 = malloc(sizeof(Property));
    if (p1 == NULL) {
        return;
    }

    strncpy(p1->name, prop_name, MAX_NAME_LEN - 1);
    strncpy(p1->type, prop_type, MAX_TYPE_LEN - 1);
    e->properties[e->num_properties] = p1;
    e->num_properties += 1;
    e->height += 1;
    if ((int)strlen(p1->name) + ((int)strlen(p1->type)) > (e->width - 2)) {
        e->width += strlen(p1->name) + strlen(p1->type);
    }

    // [ ] TODO:  Alert user that addition done
}

void addPropertyRelationship(Relationship *r, const char *prop_name,
                             const char *prop_type) {
    if (r->num_properties >= MAX_PROPERTIES) {
        return;
    }

    Property *p1 = malloc(sizeof(Property));
    if (p1 == NULL) {
        return;
    }

    strncpy(p1->name, prop_name, MAX_NAME_LEN - 1);
    strncpy(p1->type, prop_type, MAX_TYPE_LEN - 1);
    r->properties[r->num_properties] = p1;
    r->num_properties += 1;
    r->height += 1;
    if ((int)strlen(p1->name) + ((int)strlen(p1->type)) > (r->width - 2)) {
        r->width += strlen(p1->name) + strlen(p1->type);
    }

    // [ ] TODO:  Alert user that addition done
}

void tokenizeCardinalityInput(const char *input, char *card1, char *card2) {
    char buffer[10];
    strncpy(buffer, input, 9);
    buffer[9] = '\0';

    char *token = strtok(buffer, ",");
    int part = 0;

    while (token && part < 4) {
        if (part == 0)
            card1[0] = token[0];
        if (part == 1)
            card1[2] = token[0];
        if (part == 2)
            card2[0] = token[0];
        if (part == 3)
            card2[2] = token[0];
        part++;
        token = strtok(NULL, ",");
    }

    // add comas
    card1[1] = card2[1] = ',';
    card1[3] = card2[3] = '\0';
}

void addCardinality(const char *input, Cardinality *c1, Cardinality *c2) {
    // 1,n

    if (!input || strlen(input) < 7) {
        // invalid input
        // assign default values
        strcpy(c1->value, "x,x");
        strcpy(c2->value, "y,y");
        return;
    }
    char card1[4], card2[4];
    tokenizeCardinalityInput(input, card1, card2);
    strncpy(c1->value, card1, CARDINALITY_LEN - 1);
    strncpy(c2->value, card2, CARDINALITY_LEN - 1);
    c1->value[CARDINALITY_LEN - 1] = '\0';
    c2->value[CARDINALITY_LEN - 1] = '\0';
}

void addCardinalityAPI(const char *input, Relationship *r) {
    Cardinality *c = malloc(sizeof(Cardinality) * 2);
    if (!c) {
        exit(1);
    }
    addCardinality(input, &c[0], &c[1]);
    c[0].value[CARDINALITY_LEN - 1] = '\0';
    c[1].value[CARDINALITY_LEN - 1] = '\0';
    r->cards[0] = &c[0];
    r->cards[1] = &c[1];
}

Relationship *addRelationship(int x, int y, Entity *e1, Entity *e2,
                              const char *name) {
    if (!e1 || !e2)
        return NULL;

    Relationship *r = malloc(sizeof(Relationship));
    if (!r)
        return NULL;

    r->x = x;
    r->y = y;
    r->height = RELATIONSHIP_HEIGHT;
    r->width = RELATIONSHIP_WIDTH;
    r->e1 = e1;
    r->e2 = e2;
    r->num_properties = 0;
    strncpy(r->name, name, MAX_NAME_LEN - 1);
    r->name[MAX_NAME_LEN - 1] = '\0';

    for (int i = 0; i < MAX_PROPERTIES; i++) {
        r->properties[i] = NULL;
    }
    for (int i = 0; i < 2; i++) {
        r->cards[i] = NULL;
    }

    register_relationship(r); // Register the relationship globally
    return r;
}

AttachPoint findBestAttachPoint(int box_x, int box_y, int box_width,
                                int box_height, int target_x, int target_y) {
    AttachPoint candidates[4];

    candidates[0].x = box_x + box_width / 2;
    candidates[0].y = box_y;
    candidates[0].side = SIDE_TOP;

    candidates[1].x = box_x + box_width / 2;
    candidates[1].y = box_y + box_height - 1;
    candidates[1].side = SIDE_BOTTOM;

    candidates[2].x = box_x;
    candidates[2].y = box_y + box_height / 2;
    candidates[2].side = SIDE_LEFT;

    candidates[3].x = box_x + box_width - 1;
    candidates[3].y = box_y + box_height / 2;
    candidates[3].side = SIDE_RIGHT;

    int min_distance = 999999;
    AttachPoint best = candidates[0];

    for (int i = 0; i < 4; i++) {
        int dx = abs(candidates[i].x - target_x);
        int dy = abs(candidates[i].y - target_y);
        int distance = dx + dy;

        if (distance < min_distance) {
            min_distance = distance;
            best = candidates[i];
        }
    }

    return best;
}
