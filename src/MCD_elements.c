// == CHANGES : ==
// - addProperty(): the width-growth check now also reserves 1 extra column
//   for the FOREIGN_KEY "*" prefix drawn by drawEntity(), so a FK property
//   right at the width boundary doesn't get clipped.
// - search_entity()/search_relationship(): added a NULL check on the array
//   slot before dereferencing it. global_objects.relationships[]/.entities[]
//   can now contain NULL holes (unregister_relationship() leaves them after
//   an MLD conversion), and these two loops were the only ones in the
//   codebase that didn't already guard against that (graphics.c's drawing/
//   A* loops already did).
#include "MCD_elements.h"
#include "global_objects.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

Entity *createEntity(const char *name, int x, int y) {
    Entity *e = malloc(sizeof(Entity));
    if (!e)
        return NULL;

    strncpy(e->name, name, MAX_NAME_LEN - 1);
    e->name[MAX_NAME_LEN - 1] = '\0';

    int screen_height, screen_width;
    getmaxyx(stdscr, screen_height, screen_width);

    int console_height = 5;
    int gap = 3;
    int console_top_y = screen_height - console_height;

    if (y + ENTITY_HEIGHT > console_top_y - gap) {
        y = (console_top_y - gap) - ENTITY_HEIGHT;
    }
    if (y < 1) {
        y = 1;
    }

    int max_allowable_x = screen_width - ENTITY_WIDTH - gap;
    if (x > max_allowable_x) {
        x = max_allowable_x;
    }
    if (x < 1) {
        x = 1;
    }
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

bool addProperty(Entity *e, const char *prop_name, const char *prop_type, KeyType keytype) {
    if (e->num_properties >= MAX_PROPERTIES) {
        return false;
    }

    for (int i = 0; i < e->num_properties; i++) {
        if (strcmp(prop_name, e->properties[i]->name) == 0) {
            return false;
        }
    }

    Property *p1 = malloc(sizeof(Property));
    if (p1 == NULL) {
        return false;
    }

    strncpy(p1->name, prop_name, MAX_NAME_LEN - 1);
    strncpy(p1->type, prop_type, MAX_TYPE_LEN - 1);
    p1->keytype = keytype;
    e->properties[e->num_properties] = p1;
    e->num_properties += 1;
    e->height += 1;
    int key_prefix_len = (keytype == FOREIGN_KEY) ? 1 : 0;
    if ((int)strlen(p1->name) + ((int)strlen(p1->type)) + key_prefix_len > (e->width - 2)) {
        e->width += strlen(p1->name) + strlen(p1->type) + key_prefix_len;
    }
    return true;
}

bool addPropertyRelationship(Relationship *r, const char *prop_name, const char *prop_type, KeyType keytype) {
    if (r->num_properties >= MAX_PROPERTIES) {
        return false;
    }

    for (int i = 0; i < r->num_properties; i++) {
        if (strcmp(prop_name, r->properties[i]->name) == 0) {
            // add func expected()  to alert user
            return false;
        }
    }

    Property *p1 = malloc(sizeof(Property));
    if (p1 == NULL) {
        return false;
    }

    strncpy(p1->name, prop_name, MAX_NAME_LEN - 1);
    strncpy(p1->type, prop_type, MAX_TYPE_LEN - 1);
    p1->keytype = keytype;
    r->properties[r->num_properties] = p1;
    r->num_properties += 1;
    r->height += 1;
    if ((int)strlen(p1->name) + ((int)strlen(p1->type)) > (r->width - 2)) {
        r->width += strlen(p1->name) + strlen(p1->type);
    }
    return true;
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
    // "1,n,n,1"

    if (!input || strlen(input) < 7) {
        // invalid input
        // assign default values
        strcpy(c1->value, "x,x");
        strcpy(c2->value, "y,y");
        return;
    }
    char card1[4], card2[4];
    tokenizeCardinalityInput(input, card1, card2);

    // if the order is reversed we switch it
    if ((int)card1[2] < (int)card1[0]) {
        char temp_card = card1[2];
        card1[2] = card1[0];
        card1[0] = temp_card;
    }

    if ((int)card2[2] < (int)card2[0]) {
        char temp_card = card2[2];
        card2[2] = card2[0];
        card2[0] = temp_card;
    }

    strncpy(c1->value, card1, CARDINALITY_LEN - 1);
    strncpy(c2->value, card2, CARDINALITY_LEN - 1);
    c1->value[CARDINALITY_LEN - 1] = '\0';
    c2->value[CARDINALITY_LEN - 1] = '\0';
}

bool addCardinalityAPI(const char *input, Relationship *r) {
    Cardinality *c = malloc(sizeof(Cardinality) * 2);
    if (!c) {
        return false;
    }
    addCardinality(input, &c[0], &c[1]);
    c[0].value[CARDINALITY_LEN - 1] = '\0';
    c[1].value[CARDINALITY_LEN - 1] = '\0';
    r->cards[0] = &c[0];
    r->cards[1] = &c[1];
    return true;
}

Relationship *addRelationship(int x, int y, Entity *e1, Entity *e2, const char *name) {
    if (!e1 || !e2)
        return NULL;

    Relationship *r = malloc(sizeof(Relationship));
    if (!r)
        return NULL;

    int screen_height, screen_width;
    getmaxyx(stdscr, screen_height, screen_width);

    int console_height = 5;
    int gap = 2;

    int console_top_y = screen_height - console_height;

    if (y + RELATIONSHIP_HEIGHT > console_top_y - gap) {
        y = (console_top_y - gap) - RELATIONSHIP_HEIGHT;
    }

    if (y < 0) {
        y = 0;
    }

    int max_x = screen_width - RELATIONSHIP_WIDTH - gap;
    if (x > max_x) {
        x = max_x;
    }

    if (x < 0) {
        x = 0;
    }

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

    register_relationship(r);
    return r;
}

AttachPoint findBestAttachPoint(int box_x, int box_y, int box_width, int box_height, int target_x, int target_y) {
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

// Case insensitive
// TODO: implement own strcasecmp to avoid redundant dependencies for non-linux
// OS

Entity *search_entity(const char *name) {
    for (int i = 0; i < global_objects.entity_count; i++) {
        if (global_objects.entities[i] && strcasecmp(name, global_objects.entities[i]->name) == 0) {
            return global_objects.entities[i];
        }
    }
    return NULL;
}

Relationship *search_relationship(const char *name) {
    for (int i = 0; i < global_objects.relationship_count; i++) {
        if (global_objects.relationships[i] && strcasecmp(name, global_objects.relationships[i]->name) == 0) {
            return global_objects.relationships[i];
        }
    }
    return NULL;
}
