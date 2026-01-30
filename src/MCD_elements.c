#include "MCD_elements.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Entity *createEntity(const char *name, int x, int y) {
    Entity *e = malloc(sizeof(Entity));
    if (!e) {
        return NULL;
    }

    strncpy(e->name, name, MAX_NAME_LEN - 1);
    e->name[MAX_NAME_LEN - 1] = '\0';

    e->x = x;
    e->y = y;
    e->height = 10;
    e->width = 22;
    e->num_properties = 0;

    for (int i = 0; i < MAX_PROPERTIES; i++) {
        e->properties[i] = NULL;
    }

    // Alert user that creation done

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
    if (!e1 || !e2) {
        return NULL;
    }

    Relationship *r = malloc(sizeof(Relationship));
    if (!r) {
        return NULL;
    }
    r->x = x;
    r->y = y;
    r->height = 10;
    r->width = 22;
    r->e1 = e1;
    r->e2 = e2;
    r->num_properties = 0;
    strncpy(r->name, name, MAX_NAME_LEN - 1);
    r->name[MAX_NAME_LEN - 1] = '\0';

    // Init with NULL
    for (int i = 0; i < MAX_PROPERTIES; i++) {
        r->properties[i] = NULL;
    }
    for (int i = 0; i < 2; i++) {
        r->cards[i] = NULL;
    }
    return r;
}
