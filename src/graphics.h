#ifndef GRAPHICS_H
#define GRAPHICS_H
#include "DSA/astar.h"
#include "MCD_elements.h"
#include "global_objects.h"
#include <ncurses.h>

#define CONSOLE_HEIGHT 6

typedef enum status { Typing, Editing, Help } status;

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
WINDOW *create_console_window();
void draw_console_prompt(WINDOW *console_win, const char *input, status status);
void draw_all_entities(GlobalObjects global_objects, int moving_index, bool is_moving);
void draw_all_relationships(GlobalObjects global_objects, int moving_index, bool is_moving);

#endif
