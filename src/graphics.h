#ifndef GRAPHICS_H
#define GRAPHICS_H
#include "DSA/astar.h"
#include "MCD_elements.h"
#include "global_objects.h"
#include <ncurses.h>

void drawEntity(Entity *e);
void drawRelationship(Relationship *r);
void draw_hline_at(int y, int x1, int x2, chtype ch);
void draw_vline_at(int x, int y1, int y2, chtype ch);
void drawConnection(Relationship *r);
void drawConnectionAStar(Relationship *r);
void initColors();
void debugPrintPath(AStarPath *path, const char *name);
AStarPath *smooth_path(AStarPath *path);
void draw_path_with_corners(AStarPath *path);
#endif
