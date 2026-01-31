#include "MCD_elements.h"
#include <ncurses.h>

void drawEntity(Entity *e);
void drawRelationship(Relationship *r);
void draw_hline_at(int y, int x1, int x2, chtype ch);
void draw_vline_at(int x, int y1, int y2, chtype ch);
void drawConnection(Relationship *r);
void initColors();
