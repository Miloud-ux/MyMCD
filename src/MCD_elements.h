// - Added `references` field to the Property struct.  This field is only
//   meaningful when keytype == FOREIGN_KEY.  It stores the name of the entity
//   that this foreign key column points to (e.g. "Customer"), filled in by
//   migrate_foreign_key() and create_junction_entity() in parse.c during MLD
//   conversion.  It is used by generate_sql() in utils/sql.c to emit correct
//   ALTER TABLE ... REFERENCES statements.  The field is MAX_NAME_LEN bytes
//   and is zero-initialised by malloc in addProperty() / addPropertyRelationship()
//   so existing code that doesn't set it gets an empty string, which generate_sql()
//   treats as "no reference known" and skips the ALTER TABLE for that property.
// - addProperty() and addPropertyRelationship() signatures are unchanged;
//   a new helper set_property_reference() is added to fill the field after
//   the property is created.
#pragma once
#include <stdbool.h>
#define MAX_NAME_LEN 15 // old val = 14
#define MAX_TYPE_LEN 7
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
        char references[MAX_NAME_LEN];        // entity name this FK points to
        char references_column[MAX_NAME_LEN]; // PK column name in referenced entity (for SQL)
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

int mcd_strcasecmp(const char *a, const char *b);
void tokenizeCardinalitySide(const char *input, char *card);
void addCardinalitySide(const char *input, Cardinality *c);
bool addCardinalityForEntity(const char *entity_name, const char *input, Relationship *r);
// Set the references field on the last-added property of an entity.
// Called by migrate_foreign_key() and create_junction_entity() right after
// addProperty() to record which entity the FK points to.
void set_property_reference(Entity *e, const char *prop_name, const char *ref_entity_name);
void set_property_reference_column(Entity *e, const char *prop_name, const char *ref_column_name);
