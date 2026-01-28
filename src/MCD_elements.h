#ifndef SCR_MCD_ELEMENTS_H_
#define SCR_MCD_ELEMENTS_H_

#define MAX_NAME_LEN 50
#define MAX_TYPE_LEN 20
#define MAX_PROPERTIES 20

typedef struct {
        char name[MAX_NAME_LEN];
        char type[MAX_TYPE_LEN];
} Property;

typedef struct {
        int x, y;
        int height, width;
        char name[MAX_NAME_LEN];
        int num_properties;
        Property properties[MAX_PROPERTIES];
} Entity;

Entity *createEntity(const char *name, int x, int y);
void addProperty(Entity *e, const char *prop_name, const char *prop_type);

#endif // SCR_MCD_ELEMENTS_H_
