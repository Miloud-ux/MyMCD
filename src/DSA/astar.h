#ifndef DSA_ASTAR_H_
#define DSA_ASTAR_H_
#include <stdbool.h>
#include <stdlib.h>

typedef struct AStarNode {
        int x, y;   // pos
        int g_cost; // distance
        int h_cost; // heuristic
        int f_cost; // fina eval of the cell (g + h)

        struct AStarNode *parent; //  node we came from
        bool is_obstacle;         // Entity/Relationship
        bool in_open_set;         // candidate node : visidted & !evaluated
        bool in_closed_set;       // evaluated (done)
} AStarNode;

typedef struct {
        AStarNode **nodes;      //  nodes grid
        int width, height;      //  search area surface
        int offset_x, offset_y; //  grid pos
} AStarGrid;

typedef struct {
        int *path_x;
        int *path_y;
        int length;
} AStarPath;

AStarGrid *astar_create_grid(int start_x, int start_y, int end_x, int end_y,
                             int margin);
void astar_mark_obstacle(AStarGrid *grid, int x, int y, int width, int height);
AStarPath *astar_find_path(AStarGrid *grid, int start_x, int start_y, int end_x,
                           int end_y);
void astar_free_grid(AStarGrid *grid);

void astar_free_path(AStarPath *path);

#endif // !DEBUG
