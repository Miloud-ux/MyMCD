#include "MCD_elements.h"
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
    e->width = 20;
    e->num_properties = 0;

    return e;
}

void addProperty(Entity *e, const char *prop_name, const char *prop_type) {
    if (e->num_properties >= MAX_PROPERTIES) {
        return;
    }

    Property *p = &e->properties[e->num_properties];
    strncpy(p->name, prop_name, MAX_NAME_LEN - 1);
    strncpy(p->type, prop_type, MAX_TYPE_LEN - 1);

    e->num_properties++;
}
