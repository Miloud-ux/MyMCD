#include "astar.h"
#include "pqueue.h"
#include <limits.h>
#include <stdlib.h>

#define ASTAR_CACHE_SIZE 16

typedef struct {
  bool valid;
  bool found;
  int width, height;
  int offset_x, offset_y;
  int start_x, start_y, end_x, end_y;
  unsigned long signature;
  AStarPath path;
  int age;
} AStarCacheEntry;

static AStarCacheEntry astar_cache[ASTAR_CACHE_SIZE];
static int astar_cache_age = 0;
static unsigned long astar_cache_hits = 0;
static unsigned long astar_cache_misses = 0;

#define FNV_OFFSET 1469598103934665603UL
#define FNV_PRIME 1099511628211UL

static unsigned long fnv_mix(unsigned long hash, unsigned long v) {
  return (hash ^ v) * FNV_PRIME;
}

// Full lookup key: layout + start/end points.
static unsigned long full_signature(const AStarGrid *grid, int start_x,
                                    int start_y, int end_x, int end_y) {
  unsigned long h = grid->signature;
  h = fnv_mix(h, (unsigned long)start_x);
  h = fnv_mix(h, (unsigned long)start_y);
  h = fnv_mix(h, (unsigned long)end_x);
  h = fnv_mix(h, (unsigned long)end_y);
  return h;
}

// Recompute the grid signature from scratch (used when obstacles are cleared).
static unsigned long base_signature(const AStarGrid *grid) {
  unsigned long h = FNV_OFFSET;
  h = fnv_mix(h, (unsigned long)grid->width);
  h = fnv_mix(h, (unsigned long)grid->height);
  h = fnv_mix(h, (unsigned long)grid->offset_x);
  h = fnv_mix(h, (unsigned long)grid->offset_y);
  return h;
}

static int heuristic(int x1, int y1, int x2, int y2) {
  return abs(x1 - x2) + abs(y1 - y2);
}

static AStarPath *path_clone(const AStarPath *src) {
  AStarPath *clone = malloc(sizeof(AStarPath));
  clone->length = src->length;
  clone->path_x = malloc(src->length * sizeof(int));
  clone->path_y = malloc(src->length * sizeof(int));
  for (int i = 0; i < src->length; i++) {
    clone->path_x[i] = src->path_x[i];
    clone->path_y[i] = src->path_y[i];
  }
  return clone;
}

static void cache_free_entry(AStarCacheEntry *entry) {
  if (entry->path.path_x) {
    free(entry->path.path_x);
    entry->path.path_x = NULL;
  }
  if (entry->path.path_y) {
    free(entry->path.path_y);
    entry->path.path_y = NULL;
  }
  entry->valid = false;
  entry->found = false;
}

static AStarPath *cache_lookup(AStarGrid *grid, int start_x, int start_y,
                               int end_x, int end_y, bool *is_hit) {
  unsigned long sig = full_signature(grid, start_x, start_y, end_x, end_y);

  *is_hit = false;
  for (int i = 0; i < ASTAR_CACHE_SIZE; i++) {
    AStarCacheEntry *entry = &astar_cache[i];
    if (!entry->valid)
      continue;
    if (entry->signature != sig)
      continue;

    *is_hit = true;
    astar_cache_hits++;
    entry->age = astar_cache_age++;
    if (entry->found) {
      return path_clone(&entry->path);
    }
    return NULL;
  }
  astar_cache_misses++;
  return NULL;
}

static void cache_store(AStarGrid *grid, int start_x, int start_y, int end_x,
                        int end_y, bool found, const AStarPath *path) {
  AStarCacheEntry *victim = NULL;
  int oldest = INT_MAX;

  for (int i = 0; i < ASTAR_CACHE_SIZE; i++) {
    AStarCacheEntry *entry = &astar_cache[i];
    if (!entry->valid) {
      victim = entry;
      break;
    }
    if (entry->age < oldest) {
      oldest = entry->age;
      victim = entry;
    }
  }

  cache_free_entry(victim);

  victim->valid = true;
  victim->found = found;
  victim->width = grid->width;
  victim->height = grid->height;
  victim->offset_x = grid->offset_x;
  victim->offset_y = grid->offset_y;
  victim->start_x = start_x;
  victim->start_y = start_y;
  victim->end_x = end_x;
  victim->end_y = end_y;
  victim->signature = full_signature(grid, start_x, start_y, end_x, end_y);
  victim->age = astar_cache_age++;

  if (found && path) {
    victim->path.length = path->length;
    victim->path.path_x = malloc(path->length * sizeof(int));
    victim->path.path_y = malloc(path->length * sizeof(int));
    for (int i = 0; i < path->length; i++) {
      victim->path.path_x[i] = path->path_x[i];
      victim->path.path_y[i] = path->path_y[i];
    }
  } else {
    victim->path.length = 0;
    victim->path.path_x = NULL;
    victim->path.path_y = NULL;
  }
}

void astar_cache_clear(void) {
  for (int i = 0; i < ASTAR_CACHE_SIZE; i++) {
    cache_free_entry(&astar_cache[i]);
  }
}

void astar_cache_stats(unsigned long *hits, unsigned long *misses) {
  if (hits)
    *hits = astar_cache_hits;
  if (misses)
    *misses = astar_cache_misses;
}

void astar_cache_reset_stats(void) {
  astar_cache_hits = 0;
  astar_cache_misses = 0;
}

static AStarNode *get_node(AStarGrid *grid, int x, int y) {
  // convert screen coords to grid coords
  int grid_x = x - grid->offset_x;
  int grid_y = y - grid->offset_y;

  // check bounds
  if (grid_x < 0 || grid_x >= grid->width || grid_y < 0 ||
      grid_y >= grid->height) {
    return NULL;
  }

  return &grid->nodes[grid_y][grid_x];
}

static void get_neighbors(AStarGrid *grid, AStarNode *node,
                          AStarNode *neighbors[4]) {
  neighbors[0] = get_node(grid, node->x, node->y - 1); // Up
  neighbors[1] = get_node(grid, node->x, node->y + 1); // Down
  neighbors[2] = get_node(grid, node->x - 1, node->y); // Left
  neighbors[3] = get_node(grid, node->x + 1, node->y); // Right
}
AStarGrid *astar_create_grid(int start_x, int start_y, int end_x, int end_y,
                             int margin) {
  int min_x = (start_x < end_x ? start_x : end_x) - margin;
  int min_y = (start_y < end_y ? start_y : end_y) - margin;
  int max_x = (start_x > end_x ? start_x : end_x) + margin;
  int max_y = (start_y > end_y ? start_y : end_y) + margin;

  AStarGrid *grid = malloc(sizeof(AStarGrid));
  grid->offset_x = min_x;
  grid->offset_y = min_y;
  grid->width = max_x - min_x + 1;
  grid->height = max_y - min_y + 1;
  grid->signature = base_signature(grid);

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
void astar_mark_obstacle(AStarGrid *grid, int box_x, int box_y, int width,
                         int height) {
  for (int y = box_y; y < box_y + height; y++) {
    for (int x = box_x; x < box_x + width; x++) {
      int grid_x = x - grid->offset_x;
      int grid_y = y - grid->offset_y;

      if (grid_x >= 0 && grid_x < grid->width && grid_y >= 0 &&
          grid_y < grid->height) {
        AStarNode *n = &grid->nodes[grid_y][grid_x];
        if (!n->is_obstacle) {
          n->is_obstacle = true;
          grid->signature = fnv_mix(
              grid->signature, (unsigned long)n->x * 31UL +
                                   (unsigned long)n->y * 17UL + 1UL);
        }
      }
    }
  }
}

void astar_clear_obstacles(AStarGrid *grid) {
  for (int y = 0; y < grid->height; y++) {
    for (int x = 0; x < grid->width; x++) {
      grid->nodes[y][x].is_obstacle = false;
    }
  }
  grid->signature = base_signature(grid);
}

// Reset the mutable A* bookkeeping (costs, parents, open/closed flags) so a
// grid that was searched before can be searched again.  Obstacle flags and
// coordinates are left untouched.
static void reset_grid_state(AStarGrid *grid) {
  for (int y = 0; y < grid->height; y++) {
    for (int x = 0; x < grid->width; x++) {
      AStarNode *n = &grid->nodes[y][x];
      n->g_cost = INT_MAX;
      n->h_cost = 0;
      n->f_cost = INT_MAX;
      n->parent = NULL;
      n->in_open_set = false;
      n->in_closed_set = false;
    }
  }
}

AStarPath *astar_find_path(AStarGrid *grid, int start_x, int start_y, int end_x,
                           int end_y) {
  AStarNode *start_node = get_node(grid, start_x, start_y);
  AStarNode *end_node = get_node(grid, end_x, end_y);

  if (!start_node || !end_node || start_node->is_obstacle ||
      end_node->is_obstacle) {
    return NULL;
  }

  AStarPath *cached = NULL;
  bool cache_hit = false;
  cached = cache_lookup(grid, start_x, start_y, end_x, end_y, &cache_hit);
  if (cache_hit)
    return cached;

  reset_grid_state(grid);
  start_node = get_node(grid, start_x, start_y);
  end_node = get_node(grid, end_x, end_y);
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
      cache_store(grid, start_x, start_y, end_x, end_y, true, path);
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
  cache_store(grid, start_x, start_y, end_x, end_y, false, NULL);
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
