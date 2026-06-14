#include "astar.h"
#include "../utils/arena_allocator.h"
#include "pqueue.h"
#include <limits.h>

static int heuristic(int x1, int y1, int x2, int y2) { return abs(x1 - x2) + abs(y1 - y2); }
static AStarNode *get_node(AStarGrid *grid, int x, int y) {
    // convert screen coords to grid coords
    int grid_x = x - grid->offset_x;
    int grid_y = y - grid->offset_y;

    // check bounds
    if (grid_x < 0 || grid_x >= grid->width || grid_y < 0 || grid_y >= grid->height) {
        return NULL;
    }

    return &grid->nodes[grid_y][grid_x];
}

static void get_neighbors(AStarGrid *grid, AStarNode *node, AStarNode *neighbors[4]) {
    // Up, Down, Left, Right (no diagonals for orthogonal movement)
    neighbors[0] = get_node(grid, node->x, node->y - 1); // Up
    neighbors[1] = get_node(grid, node->x, node->y + 1); // Down
    neighbors[2] = get_node(grid, node->x - 1, node->y); // Left
    neighbors[3] = get_node(grid, node->x + 1, node->y); // Right
}
AStarGrid *astar_create_grid(int start_x, int start_y, int end_x, int end_y, int margin) {
    int min_x = (start_x < end_x ? start_x : end_x) - margin;
    int min_y = (start_y < end_y ? start_y : end_y) - margin;
    int max_x = (start_x > end_x ? start_x : end_x) + margin;
    int max_y = (start_y > end_y ? start_y : end_y) + margin;

    AStarGrid *grid = malloc(sizeof(AStarGrid));
    grid->offset_x = min_x;
    grid->offset_y = min_y;
    grid->width = max_x - min_x + 1;
    grid->height = max_y - min_y + 1;

    grid->nodes = malloc(grid->height * sizeof(AStarNode *));
    for (int y = 0; y < grid->height; y++) {
        grid->nodes[y] = malloc(grid->width * sizeof(AStarNode));
        for (int x = 0; x < grid->width; x++) {
            grid->nodes[y][x].x = grid->offset_x + x;
            grid->nodes[y][x].y = grid->offset_y + y;
            grid->nodes[y][x].g_cost = INT_MAX;
            grid->nodes[y][x].h_cost = 0;
            grid->nodes[y][x].f_cost = INT_MAX;
            grid->nodes[y][x].parent = NULL;
            grid->nodes[y][x].is_obstacle = false;
            grid->nodes[y][x].in_open_set = false;
            grid->nodes[y][x].in_closed_set = false;
        }
    }
    return grid;
}
void astar_mark_obstacle(AStarGrid *grid, int box_x, int box_y, int width, int height) {
    for (int y = box_y; y < box_y + height; y++) {
        for (int x = box_x; x < box_x + width; x++) {
            int grid_x = x - grid->offset_x;
            int grid_y = y - grid->offset_y;

            if (grid_x >= 0 && grid_x < grid->width && grid_y >= 0 && grid_y < grid->height) {
                grid->nodes[grid_y][grid_x].is_obstacle = true;
            }
        }
    }
}

AStarPath *astar_find_path(AStarGrid *grid, int start_x, int start_y, int end_x, int end_y) {
    AStarNode *start_node = get_node(grid, start_x, start_y);
    AStarNode *end_node = get_node(grid, end_x, end_y);

    if (!start_node || !end_node || start_node->is_obstacle || end_node->is_obstacle) {
        return NULL;
    }

    PriorityQueue *open_set = pq_create(grid->width * grid->height);

    start_node->g_cost = 0;
    start_node->h_cost = heuristic(start_x, start_y, end_x, end_y);
    start_node->f_cost = start_node->g_cost + start_node->h_cost;
    start_node->in_open_set = true;
    pq_push(open_set, start_node);

    while (!pq_is_empty(open_set)) {
        AStarNode *current = pq_pop(open_set);
        current->in_open_set = false;

        if (current->x == end_x && current->y == end_y) {
            AStarPath *path = malloc(sizeof(AStarPath));
            path->length = 0;
            path->path_x = NULL;
            path->path_y = NULL;

            AStarNode *node = current;
            while (node) {
                path->length++;
                node = node->parent;
            }

            path->path_x = malloc(path->length * sizeof(int));
            path->path_y = malloc(path->length * sizeof(int));

            node = current;
            int idx = path->length - 1;
            while (node) {
                path->path_x[idx] = node->x;
                path->path_y[idx] = node->y;
                node = node->parent;
                idx--;
            }

            pq_free(open_set);
            return path;
        }

        current->in_closed_set = true;

        AStarNode *neighbors[4];
        get_neighbors(grid, current, neighbors);

        for (int i = 0; i < 4; i++) {
            AStarNode *neighbor = neighbors[i];

            if (!neighbor || neighbor->is_obstacle || neighbor->in_closed_set) {
                continue;
            }

            int new_g_cost = current->g_cost + 1;

            if (new_g_cost < neighbor->g_cost || !neighbor->in_open_set) {
                if (current->parent) {
                    int prev_dir_x = current->x - current->parent->x;
                    int prev_dir_y = current->y - current->parent->y;
                    int curr_dir_x = neighbor->x - current->x;
                    int curr_dir_y = neighbor->y - current->y;

                    // if direction changes (turn detected) add penalty
                    if (!(prev_dir_x == curr_dir_x && prev_dir_y == curr_dir_y)) {
                        new_g_cost += 3; // Turn penalty to reduce staircase effect
                    }
                }

                neighbor->g_cost = new_g_cost;
                neighbor->h_cost = heuristic(neighbor->x, neighbor->y, end_x, end_y);
                neighbor->f_cost = neighbor->g_cost + neighbor->h_cost;
                neighbor->parent = current;

                if (!neighbor->in_open_set) {
                    neighbor->in_open_set = true;
                    pq_push(open_set, neighbor);
                }
            }
        }
    }

    pq_free(open_set);
    return NULL;
}

void astar_free_grid(AStarGrid *grid) {
    if (!grid)
        return;

    for (int y = 0; y < grid->height; y++) {
        free(grid->nodes[y]);
    }
    free(grid->nodes);
    free(grid);
}

void astar_free_path(AStarPath *path) {
    if (!path)
        return;

    free(path->path_x);
    free(path->path_y);
    free(path);
}
