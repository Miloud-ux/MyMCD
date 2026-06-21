#ifndef SCR_MCD_ELEMENTS_H_
#define SCR_MCD_ELEMENTS_H_

#include <stdbool.h>
#define MAX_NAME_LEN 15 // old val = 14
#define MAX_TYPE_LEN 6
#define MAX_KEY_TYPE_LEN 2
#define CARDINALITY_LEN 4
#define RAW_CARDINALITY_LEN 9
#define MAX_PROPERTIES 20

#define ENTITY_HEIGHT 5
#define ENTITY_WIDTH 18
#define RELATIONSHIP_HEIGHT 5
#define RELATIONSHIP_WIDTH 10

typedef enum { FOREIGN_KEY, PRIMARY_KEY, NORMAL_KEY } KeyType;
typedef struct {
        char name[MAX_NAME_LEN];
        char type[MAX_TYPE_LEN];
        KeyType keytype;
} Property;

typedef struct {
        int x, y;
        int height, width;
        char name[MAX_NAME_LEN];
        int num_properties;
        Property *properties[MAX_PROPERTIES];
} Entity;

typedef struct {
        char value[CARDINALITY_LEN];
} Cardinality;

typedef struct {
        int x, y;
        int height, width;
        char name[MAX_NAME_LEN];
        Entity *e1;
        Entity *e2;
        int num_properties;
        Property *properties[MAX_PROPERTIES];
        Cardinality *cards[2];
} Relationship;

typedef enum {
    SIDE_TOP,
    SIDE_BOTTOM,
    SIDE_RIGHT,
    SIDE_LEFT,
} Side;

typedef struct {
        int x;
        int y;
        Side side;
} AttachPoint;

Entity *createEntity(const char *name, int x, int y);
bool addProperty(Entity *e, const char *prop_name, const char *prop_type, KeyType keytype);
Relationship *addRelationship(int x, int y, Entity *e1, Entity *e2, const char *name);
void addCardinality(const char *input, Cardinality *c1, Cardinality *c2);
void tokenizeCardinalityInput(const char *input, char *card1, char *card2);
bool addCardinalityAPI(const char *input, Relationship *r);
bool addPropertyRelationship(Relationship *r, const char *prop_name, const char *prop_type, KeyType keytype);
AttachPoint findBestAttachPoint(int box_x, int box_y, int box_width, int box_height, int target_x, int target_y);

Entity *search_entity(const char *name);
Relationship *search_relationship(const char *name);

#endif // SCR_MCD_ELEMENTS_H_
