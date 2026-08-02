
#include "graphics.h"
#include "DSA/astar.h"
#include "DSA/kmp.h"
#include "DSA/vec.h"
#include "help_window.h"
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

enum status last_status = Typing;

void debugPrintPath(AStarPath *path, const char *name) {
    if (!path) {
        mvprintw(0, 0, "%s: No path found", name);
    } else {
        mvprintw(0, 0, "%s: Path length %d", name, path->length);
        for (int i = 0; i < path->length; i++) {
            mvprintw(1 + i, 0, "  [%d] = (%d, %d)", i, path->path_x[i], path->path_y[i]);
        }
    }
    refresh();
    getch();
}

void initColors() {
    start_color();
    // TODO : research this func for terminals that don't support colors
    // use_default_colors();
    init_pair(1, COLOR_RED, COLOR_BLACK);

    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_WHITE, COLOR_BLUE);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_WHITE);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
}

void draw_hline_at(int y, int x1, int x2, chtype ch) {
    if (x1 == x2) {
        mvaddch(y, x1, ch);
        return;
    }
    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    move(y, x1);
    hline(ch, x2 - x1 + 1);
}

void draw_vline_at(int x, int y1, int y2, chtype ch) {
    if (y1 == y2) {
        mvaddch(y1, x, ch);
        return;
    }
    if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
    }
    move(y1, x);
    vline(ch, y2 - y1 + 1);
}

/* Helper: draw a clipped horizontal line that stays inside screen bounds. */
static void safe_hline(int y, int x1, int x2, chtype ch) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    if (y < 0 || y >= max_y)
        return;
    if (x1 < 0)
        x1 = 0;
    if (x2 >= max_x)
        x2 = max_x - 1;
    if (x1 > x2)
        return;
    mvaddch(y, x1, ch);
    if (x2 > x1)
        mvhline(y, x1 + 1, ch, x2 - x1);
}

/* Helper: format a property line into a buffer, truncated to fit inside
 * 'avail' columns.  Returns the formatted string (may be truncated).
 * prefix: 1 for FK "*", 0 otherwise.
 * If the full "name:type" doesn't fit, we try "name" alone, then "na.." etc.
 */
static void format_property_line(char *out, size_t out_size, const char *name, const char *type, int prefix, int avail) {
    if (avail < 1) {
        out[0] = '\0';
        return;
    }

    int prefix_len = prefix ? 1 : 0;
    int need = prefix_len + (int)strlen(name) + 1 + (int)strlen(type); // *name:type

    if (need <= avail) {
        // Everything fits
        if (prefix)
            snprintf(out, out_size, "*%s:%s", name, type);
        else
            snprintf(out, out_size, "%s:%s", name, type);
        return;
    }

    // Try without the type suffix
    need = prefix_len + (int)strlen(name);
    if (need <= avail) {
        if (prefix)
            snprintf(out, out_size, "*%s", name);
        else
            snprintf(out, out_size, "%s", name);
        return;
    }

    // Need to truncate the name itself
    int max_name = avail - prefix_len;
    if (max_name < 1) {
        out[0] = '\0';
        return;
    }

    if (max_name >= 2) {
        // Truncate with ".."
        if (prefix)
            snprintf(out, out_size, "*%-.*s..", max_name - 2, name);
        else
            snprintf(out, out_size, "%-.*s..", max_name - 2, name);
    } else {
        // Only 1 char available
        if (prefix)
            snprintf(out, out_size, "*%-.1s", name);
        else
            snprintf(out, out_size, "%-.1s", name);
    }
}

void drawEntity(Entity *e) {
    if (!e) {
        return;
    }

    int max_scr_x;
    int max_scr_y;
    getmaxyx(stdscr, max_scr_y, max_scr_x);

    if (e->x < 0) {
        e->x = 0;
    } else if (e->x + e->width > max_scr_x) {
        int dist = (e->x + e->width) - max_scr_x;
        e->x -= dist;
    }
    if (e->y < 0) {
        e->y = 0;
    } else if (e->y + e->height + CONSOLE_HEIGHT > max_scr_y) {
        int dist = (e->y + e->height + CONSOLE_HEIGHT) - max_scr_y;
        e->y -= dist;
    }

    int interior_w = e->width - 2; // usable width inside left/right borders

    // --- Draw properties first (so we can clear interior safely) ---
    for (int i = 0; i < e->num_properties; i++) {
        if (e->properties[i]) {
            int row = e->y + 3 + i;
            // Clear the entire interior row first
            mvprintw(row, e->x + 1, "%-*s", interior_w, "");

            Property *p = e->properties[i];
            int prefix = (p->keytype == FOREIGN_KEY) ? 1 : 0;

            char line[64];
            format_property_line(line, sizeof(line), p->name, p->type, prefix, interior_w);

            attron(A_BOLD);
            if (p->keytype == PRIMARY_KEY) {
                attron(A_UNDERLINE);
                mvprintw(row, e->x + 1, "%s", line);
                attroff(A_UNDERLINE);
            } else {
                mvprintw(row, e->x + 1, "%s", line);
            }
            attroff(A_BOLD);
        }
    }

    // --- Draw borders on top (crisp, can't be overwritten by text) ---
    draw_hline_at(e->y, e->x + 1, e->x + e->width - 2, ACS_HLINE);                 // top border
    draw_vline_at(e->x, e->y + 1, e->y + e->height - 2, ACS_VLINE);                // left border
    draw_vline_at(e->x + e->width - 1, e->y + 1, e->y + e->height - 2, ACS_VLINE); // right border
    draw_hline_at(e->y + 2, e->x + 1, e->x + e->width - 2, ACS_HLINE);             // name separator
    draw_hline_at(e->y + e->height - 1, e->x + 1, e->x + e->width - 2, ACS_HLINE); // bottom border

    // Corners
    mvaddch(e->y, e->x, ACS_ULCORNER);
    mvaddch(e->y, e->x + e->width - 1, ACS_URCORNER);
    mvaddch(e->y + e->height - 1, e->x, ACS_LLCORNER);
    mvaddch(e->y + e->height - 1, e->x + e->width - 1, ACS_LRCORNER);

    // Entity Name (centered, truncated if too long)
    attron(A_BOLD);
    int name_len = (int)strlen(e->name);
    if (name_len > interior_w) {
        char truncated[64];
        snprintf(truncated, sizeof(truncated), "%.*s..", interior_w - 2, e->name);
        mvprintw(e->y + 1, e->x + 1, "%-*s", interior_w, truncated);
    } else {
        mvprintw(e->y + 1, e->x + (e->width - name_len) / 2, "%s", e->name);
    }
    attroff(A_BOLD);
}

void drawRelationship(Relationship *r) {
    if (!r) {
        return;
    }

    int max_scr_x;
    int max_scr_y;
    getmaxyx(stdscr, max_scr_y, max_scr_x);

    if (r->x < 0) {
        r->x = 0;
    } else if (r->x + r->width > max_scr_x) {
        int dist = (r->x + r->width) - max_scr_x;
        r->x -= dist;
    }
    if (r->y < 0) {
        r->y = 0;
    } else if (r->y + r->height + CONSOLE_HEIGHT > max_scr_y) {
        int dist = (r->y + r->height + CONSOLE_HEIGHT) - max_scr_y;
        r->y -= dist;
    }

    int interior_w = r->width - 2;

    // --- Draw properties first ---
    for (int i = 0; i < r->num_properties; i++) {
        if (r->properties[i]) {
            int row = r->y + 3 + i;
            mvprintw(row, r->x + 1, "%-*s", interior_w, ""); // clear interior

            Property *p = r->properties[i];
            char line[64];
            format_property_line(line, sizeof(line), p->name, p->type, 0, interior_w);

            attron(A_BOLD);
            if (p->keytype == PRIMARY_KEY) {
                attron(A_UNDERLINE);
                mvprintw(row, r->x + 1, "%s", line);
                attroff(A_UNDERLINE);
            } else {
                mvprintw(row, r->x + 1, "%s", line);
            }
            attroff(A_BOLD);
        }
    }

    // --- Draw borders on top ---
    // Top border
    safe_hline(r->y, r->x + 1, r->x + r->width - 2, ACS_HLINE);
    // Bottom border
    safe_hline(r->y + r->height - 1, r->x + 1, r->x + r->width - 2, ACS_HLINE);
    // Left border
    for (int i = 1; i < r->height - 1; i++) {
        if (r->y + i >= 0 && r->y + i < max_scr_y && r->x >= 0 && r->x < max_scr_x)
            mvaddch(r->y + i, r->x, ACS_VLINE);
    }
    // Right border
    for (int i = 1; i < r->height - 1; i++) {
        if (r->y + i >= 0 && r->y + i < max_scr_y && r->x + r->width - 1 >= 0 && r->x + r->width - 1 < max_scr_x)
            mvaddch(r->y + i, r->x + r->width - 1, ACS_VLINE);
    }

    // Name separator
    safe_hline(r->y + 2, r->x + 1, r->x + r->width - 2, ACS_HLINE);

    // Corners (diamonds for MCD notation)
    if (r->y >= 0 && r->y < max_scr_y && r->x >= 0 && r->x < max_scr_x)
        mvaddch(r->y, r->x, ACS_DIAMOND);
    if (r->y >= 0 && r->y < max_scr_y && r->x + r->width - 1 >= 0 && r->x + r->width - 1 < max_scr_x)
        mvaddch(r->y, r->x + r->width - 1, ACS_DIAMOND);
    if (r->y + r->height - 1 >= 0 && r->y + r->height - 1 < max_scr_y && r->x >= 0 && r->x < max_scr_x)
        mvaddch(r->y + r->height - 1, r->x, ACS_DIAMOND);
    if (r->y + r->height - 1 >= 0 && r->y + r->height - 1 < max_scr_y && r->x + r->width - 1 >= 0 &&
        r->x + r->width - 1 < max_scr_x)
        mvaddch(r->y + r->height - 1, r->x + r->width - 1, ACS_DIAMOND);

    // Relationship Name (centered, truncated)
    attron(A_BOLD);
    int name_len = (int)strlen(r->name);
    if (name_len > interior_w) {
        char truncated[64];
        snprintf(truncated, sizeof(truncated), "%.*s..", interior_w - 2, r->name);
        mvprintw(r->y + 1, r->x + 1, "%-*s", interior_w, truncated);
    } else {
        mvprintw(r->y + 1, r->x + (r->width - name_len) / 2, "%s", r->name);
    }
    attroff(A_BOLD);
}

void add_endpoints_to_path(AStarPath *path, int start_x, int start_y, int end_x, int end_y) {
    if (!path)
        return;

    int new_len = path->length + 2;
    int *new_x = malloc(new_len * sizeof(int));
    int *new_y = malloc(new_len * sizeof(int));

    if (!new_x || !new_y)
        return;

    // start points (Border Point)
    new_x[0] = start_x;
    new_y[0] = start_y;

    // copy the existing A* path
    for (int i = 0; i < path->length; i++) {
        new_x[i + 1] = path->path_x[i];
        new_y[i + 1] = path->path_y[i];
    }

    // append the actual End (Border Point)
    new_x[new_len - 1] = end_x;
    new_y[new_len - 1] = end_y;

    // replace the old arrays
    free(path->path_x);
    free(path->path_y);
    path->path_x = new_x;
    path->path_y = new_y;
    path->length = new_len;
}
void draw_path_with_corners(AStarPath *path) {
    if (!path || path->length < 2)
        return;

    attron(COLOR_PAIR(3));

    //  draw the straight lines first
    for (int i = 0; i < path->length - 1; i++) {
        int x1 = path->path_x[i];
        int y1 = path->path_y[i];
        int x2 = path->path_x[i + 1];
        int y2 = path->path_y[i + 1];

        if (x1 == x2) {
            draw_vline_at(x1, y1, y2, ACS_VLINE);
        } else if (y1 == y2) {
            draw_hline_at(y1, x1, x2, ACS_HLINE);
        }
    }

    // draw corners on top of the joints
    for (int i = 1; i < path->length - 1; i++) {
        int prev_x = path->path_x[i - 1];
        int prev_y = path->path_y[i - 1];
        int curr_x = path->path_x[i];
        int curr_y = path->path_y[i];
        int next_x = path->path_x[i + 1];
        int next_y = path->path_y[i + 1];

        //  check donnectivity
        // We look at the Previous and Next nodes to see where the neighbors
        // are.
        bool has_up = (prev_y < curr_y) || (next_y < curr_y);
        bool has_down = (prev_y > curr_y) || (next_y > curr_y);
        bool has_left = (prev_x < curr_x) || (next_x < curr_x);
        bool has_right = (prev_x > curr_x) || (next_x > curr_x);

        // select character based on Neighbors
        attron(A_BOLD);
        if (has_down && has_right) {
            mvaddch(curr_y, curr_x, ACS_ULCORNER); // ┌
        } else if (has_down && has_left) {
            mvaddch(curr_y, curr_x, ACS_URCORNER); // ┐
        } else if (has_up && has_right) {
            mvaddch(curr_y, curr_x, ACS_LLCORNER); // └
        } else if (has_up && has_left) {
            mvaddch(curr_y, curr_x, ACS_LRCORNER); // ┘
        }
        attroff(A_BOLD);
    }

    attroff(COLOR_PAIR(3));
}

// Cardinality/line-overlap fix.
//  These two helpers spread multiple relationships evenly along the
// side they share, so each one gets its own point, right next to its own line,
// close to the entity. findBestAttachPoint() itself and MCD_elements.c are
// untouched, since other code may depend on its current behavior.
static void get_attach_slot(Entity *e, Side side, Relationship *r, int *slot_index, int *slot_total) {
    int idx = 0;
    int total = 0;

    for (int i = 0; i < global_objects.relationship_count; i++) {
        Relationship *other = global_objects.relationships[i];
        if (!other) {
            continue;
        }
        if (other->e1 != e && other->e2 != e) {
            continue; // doesn't touch this entity at all
        }

        AttachPoint other_ap = findBestAttachPoint(e->x, e->y, e->width, e->height, other->x, other->y);
        if (other_ap.side != side) {
            continue; // touches this entity, but on a different side
        }

        if (other == r) {
            idx = total;
        }
        total++;
    }

    *slot_index = idx;
    *slot_total = (total > 0) ? total : 1;
}

static AttachPoint slotted_attach_point(Entity *e, Side side, int slot_index, int slot_total) {
    AttachPoint p;
    p.side = side;

    if (side == SIDE_TOP || side == SIDE_BOTTOM) {
        int usable = e->width - 2; // stay inside the corner characters
        if (usable < 1) {
            usable = 1;
        }
        int step = usable / (slot_total + 1);
        if (step < 1) {
            step = 1;
        }
        p.x = e->x + 1 + step * (slot_index + 1);
        if (p.x > e->x + e->width - 2) {
            p.x = e->x + e->width - 2;
        }
        p.y = (side == SIDE_TOP) ? e->y : e->y + e->height - 1;
    } else {
        int usable = e->height - 2;
        if (usable < 1) {
            usable = 1;
        }
        int step = usable / (slot_total + 1);
        if (step < 1) {
            step = 1;
        }
        p.y = e->y + 1 + step * (slot_index + 1);
        if (p.y > e->y + e->height - 2) {
            p.y = e->y + e->height - 2;
        }
        p.x = (side == SIDE_LEFT) ? e->x : e->x + e->width - 1;
    }

    return p;
}

// ---------------------------------------------------------------------------
// Persisted A* grid cache.
//
// Each relationship draws two A* routes (entity1 -> relationship and
// relationship -> entity2).  Creating + freeing both grids on every frame is
// pure waste: the grid geometry only changes when the attach points move, and
// the obstacle layout only changes when an entity/relationship is added,
// removed or moved.  We therefore keep the grids alive across frames and only
// (re)build them when something actually changed.
// ---------------------------------------------------------------------------
#define REL_GRID_CACHE_SIZE 64

typedef struct {
    Relationship *rel;        // which relationship these grids belong to
    AStarGrid *grid1;
    AStarGrid *grid2;
    AStarPath *path1;         // final path (endpoints injected) for route 1
    AStarPath *path2;         // final path (endpoints injected) for route 2
    int s1x, s1y, e1x, e1y;   // geometry the grids were built for (grid1)
    int s2x, s2y, e2x, e2y;   // geometry the grids were built for (grid2)
    bool in_use;
    int age;                  // LRU counter
} RelGridCacheEntry;

static RelGridCacheEntry rel_grid_cache[REL_GRID_CACHE_SIZE];
static int rel_grid_cache_age = 0;
static unsigned long grid_rebuilds = 0;
static unsigned long grid_reuses = 0;
static long last_frame_us = 0;
static long frame_erase_us = 0;
static long frame_entities_us = 0;
static long frame_relationships_us = 0;
static long frame_asts_calls = 0;

// ---------------------------------------------------------------------------
// Per-frame change detection for the route grid cache.
//
// Instead of a global "world signature" that invalidates every relationship's
// grids whenever anything moves, we track which boxes actually changed since
// the last drawn frame (position diffs + register/unregister generation).
// A route only rebuilds when its own grid bounding box intersects a changed
// box, so moving one element only rebuilds the routes that are actually near
// it.
// ---------------------------------------------------------------------------
typedef struct {
    int x, y, w, h;
} DirtyBox;

#define MAX_DIRTY_BOXES (MAX_OBJECTS * 4)

static DirtyBox frame_dirty[MAX_DIRTY_BOXES];
static int frame_dirty_count = 0;
static bool frame_force_rebuild = false; // structural change this frame

// Last-frame position snapshot so moves can be detected by diffing.
static Entity *last_entities[MAX_OBJECTS];
static int last_ew[MAX_OBJECTS];
static int last_eh[MAX_OBJECTS];
static int last_ex[MAX_OBJECTS];
static int last_ey[MAX_OBJECTS];
static int last_ec = -1;
static Relationship *last_rels[MAX_OBJECTS];
static int last_rw[MAX_OBJECTS];
static int last_rh[MAX_OBJECTS];
static int last_rx[MAX_OBJECTS];
static int last_ry[MAX_OBJECTS];
static int last_rc = -1;
static int last_gen = -1;

static void add_dirty_box(int x, int y, int w, int h) {
    if (frame_dirty_count < MAX_DIRTY_BOXES) {
        frame_dirty[frame_dirty_count].x = x;
        frame_dirty[frame_dirty_count].y = y;
        frame_dirty[frame_dirty_count].w = w;
        frame_dirty[frame_dirty_count].h = h;
        frame_dirty_count++;
    }
}

static void detect_changes(void) {
    frame_dirty_count = 0;

    bool structural = (world_generation != last_gen) ||
                      (global_objects.entity_count != last_ec) ||
                      (global_objects.relationship_count != last_rc);
    last_gen = world_generation;
    frame_force_rebuild = structural;

    if (!structural) {
        // Entities: current box (or both old+new box on move) into the dirty set.
        for (int i = 0; i < global_objects.entity_count; i++) {
            Entity *e = global_objects.entities[i];
            if (!e)
                continue;
            int j;
            for (j = 0; j < last_ec; j++) {
                if (last_entities[j] == e)
                    break;
            }
            if (j < last_ec) {
                if (last_ex[j] != e->x || last_ey[j] != e->y) {
                    add_dirty_box(last_ex[j], last_ey[j], last_ew[j], last_eh[j]);
                    add_dirty_box(e->x, e->y, e->width, e->height);
                }
            } else {
                add_dirty_box(e->x, e->y, e->width, e->height); // newly added
            }
        }
        // Entities removed since last frame: mark their old box.
        for (int j = 0; j < last_ec; j++) {
            if (!last_entities[j])
                continue;
            bool present = false;
            for (int i = 0; i < global_objects.entity_count; i++) {
                if (global_objects.entities[i] == last_entities[j]) {
                    present = true;
                    break;
                }
            }
            if (!present)
                add_dirty_box(last_ex[j], last_ey[j], last_ew[j], last_eh[j]);
        }

        // Relationships: same diff.
        for (int i = 0; i < global_objects.relationship_count; i++) {
            Relationship *r = global_objects.relationships[i];
            if (!r)
                continue;
            int j;
            for (j = 0; j < last_rc; j++) {
                if (last_rels[j] == r)
                    break;
            }
            if (j < last_rc) {
                if (last_rx[j] != r->x || last_ry[j] != r->y) {
                    add_dirty_box(last_rx[j], last_ry[j], last_rw[j], last_rh[j]);
                    add_dirty_box(r->x, r->y, r->width, r->height);
                }
            } else {
                add_dirty_box(r->x, r->y, r->width, r->height); // newly added
            }
        }
        for (int j = 0; j < last_rc; j++) {
            if (!last_rels[j])
                continue;
            bool present = false;
            for (int i = 0; i < global_objects.relationship_count; i++) {
                if (global_objects.relationships[i] == last_rels[j]) {
                    present = true;
                    break;
                }
            }
            if (!present)
                add_dirty_box(last_rx[j], last_ry[j], last_rw[j], last_rh[j]);
        }
    }

    // Refresh the position snapshot for the next frame.
    last_ec = global_objects.entity_count;
    for (int i = 0; i < global_objects.entity_count; i++) {
        Entity *e = global_objects.entities[i];
        last_entities[i] = e;
        if (e) {
            last_ex[i] = e->x;
            last_ey[i] = e->y;
            last_ew[i] = e->width;
            last_eh[i] = e->height;
        }
    }
    last_rc = global_objects.relationship_count;
    for (int i = 0; i < global_objects.relationship_count; i++) {
        Relationship *r = global_objects.relationships[i];
        last_rels[i] = r;
        if (r) {
            last_rx[i] = r->x;
            last_ry[i] = r->y;
            last_rw[i] = r->width;
            last_rh[i] = r->height;
        }
    }
}

// True when the route between (sx,sy) and (ex,ey) may be affected by this
// frame's changes: its A* grid bounding box intersects a changed box.
static bool route_affected(int sx, int sy, int ex, int ey) {
    if (frame_force_rebuild)
        return true;
    if (frame_dirty_count == 0)
        return false;

    int margin = 10;
    int x0 = (sx < ex ? sx : ex) - margin;
    int x1 = (sx > ex ? sx : ex) + margin;
    int y0 = (sy < ey ? sy : ey) - margin;
    int y1 = (sy > ey ? sy : ey) + margin;

    for (int i = 0; i < frame_dirty_count; i++) {
        const DirtyBox *b = &frame_dirty[i];
        if (b->x <= x1 && b->x + b->w >= x0 && b->y <= y1 && b->y + b->h >= y0)
            return true;
    }
    return false;
}

// Find the cache slot for a relationship, or an empty slot to fill.
static RelGridCacheEntry *rel_grid_cache_find(Relationship *rel) {
    for (int i = 0; i < REL_GRID_CACHE_SIZE; i++) {
        if (rel_grid_cache[i].in_use && rel_grid_cache[i].rel == rel)
            return &rel_grid_cache[i];
    }

    // Not cached: pick an empty slot, else evict the LRU one.
    RelGridCacheEntry *empty = NULL;
    for (int i = 0; i < REL_GRID_CACHE_SIZE; i++) {
        if (!rel_grid_cache[i].in_use) {
            empty = &rel_grid_cache[i];
            break;
        }
    }
    if (empty) {
        empty->rel = rel;
        empty->in_use = true;
        return empty;
    }

    int oldest = INT_MAX;
    RelGridCacheEntry *victim = NULL;
    for (int i = 0; i < REL_GRID_CACHE_SIZE; i++) {
        if (rel_grid_cache[i].age < oldest) {
            oldest = rel_grid_cache[i].age;
            victim = &rel_grid_cache[i];
        }
    }
    if (victim) {
        astar_free_grid(victim->grid1);
        astar_free_grid(victim->grid2);
        astar_free_path(victim->path1);
        astar_free_path(victim->path2);
        victim->grid1 = NULL;
        victim->grid2 = NULL;
        victim->path1 = NULL;
        victim->path2 = NULL;
        victim->rel = rel;
        return victim;
    }
    return NULL;
}

static void rel_grid_cache_touch(RelGridCacheEntry *e) {
    e->age = rel_grid_cache_age++;
}

// Mark every entity/relationship box as an obstacle in the given grid.
static void mark_all_obstacles(AStarGrid *g) {
    for (int i = 0; i < global_objects.entity_count; i++) {
        if (global_objects.entities[i])
            astar_mark_obstacle(g, global_objects.entities[i]->x,
                                global_objects.entities[i]->y,
                                global_objects.entities[i]->width,
                                global_objects.entities[i]->height);
    }
    for (int i = 0; i < global_objects.relationship_count; i++) {
        if (global_objects.relationships[i])
            astar_mark_obstacle(g, global_objects.relationships[i]->x,
                                global_objects.relationships[i]->y,
                                global_objects.relationships[i]->width,
                                global_objects.relationships[i]->height);
    }
}

// Build a grid for the given route and mark all obstacles into it.
static AStarGrid *build_route_grid(int sx, int sy, int ex, int ey) {
    int margin = 10;
    AStarGrid *g = astar_create_grid(sx, sy, ex, ey, margin);
    mark_all_obstacles(g);
    return g;
}

// Build/refresh the cached grids for a relationship.  Returns a fully-built
// grid1 and grid2 via out-params, ready for astar_find_path().  Returns true
// when both grids (and therefore the cached paths) were reused unchanged.
//
// A grid is reused when its geometry is unchanged AND the route was not
// affected by this frame's changes.  If geometry is unchanged but the route
// is affected, the grid allocation is kept and only its obstacles are
// refreshed (avoids the per-grid malloc churn).  A geometry change rebuilds
// the grid from scratch.
static bool ensure_route_grids(RelGridCacheEntry *e, int s1x, int s1y, int e1x,
                               int e1y, int s2x, int s2y, int e2x, int e2y,
                               AStarGrid **grid1, AStarGrid **grid2) {
    bool geom1 = e->grid1 && e->s1x == s1x && e->s1y == s1y &&
                 e->e1x == e1x && e->e1y == e1y;
    bool geom2 = e->grid2 && e->s2x == s2x && e->s2y == s2y &&
                 e->e2x == e2x && e->e2y == e2y;
    bool affected1 = route_affected(s1x, s1y, e1x, e1y);
    bool affected2 = route_affected(s2x, s2y, e2x, e2y);

    bool reused1 = geom1 && !affected1;
    if (reused1) {
        grid_reuses++;
    } else if (geom1) {
        // Route is near a change: keep the grid, refresh its obstacles.
        astar_clear_obstacles(e->grid1);
        mark_all_obstacles(e->grid1);
        astar_free_path(e->path1);
        e->path1 = NULL;
        grid_rebuilds++;
    } else {
        astar_free_grid(e->grid1);
        astar_free_path(e->path1);
        e->grid1 = build_route_grid(s1x, s1y, e1x, e1y);
        e->path1 = NULL;
        e->s1x = s1x;
        e->s1y = s1y;
        e->e1x = e1x;
        e->e1y = e1y;
        grid_rebuilds++;
    }

    bool reused2 = geom2 && !affected2;
    if (reused2) {
        grid_reuses++;
    } else if (geom2) {
        astar_clear_obstacles(e->grid2);
        mark_all_obstacles(e->grid2);
        astar_free_path(e->path2);
        e->path2 = NULL;
        grid_rebuilds++;
    } else {
        astar_free_grid(e->grid2);
        astar_free_path(e->path2);
        e->grid2 = build_route_grid(s2x, s2y, e2x, e2y);
        e->path2 = NULL;
        e->s2x = s2x;
        e->s2y = s2y;
        e->e2x = e2x;
        e->e2y = e2y;
        grid_rebuilds++;
    }

    *grid1 = e->grid1;
    *grid2 = e->grid2;
    return reused1 && reused2 && e->path1 && e->path2;
}

void drawConnectionAStar(Relationship *r) {
    if (!r || !r->e1 || !r->e2)
        return;

    AttachPoint ap1 = findBestAttachPoint(r->e1->x, r->e1->y, r->e1->width, r->e1->height, r->x, r->y);
    AttachPoint ap_rel_e1 = findBestAttachPoint(r->x, r->y, r->width, r->height, r->e1->x, r->e1->y);
    AttachPoint ap_rel_e2 = findBestAttachPoint(r->x, r->y, r->width, r->height, r->e2->x, r->e2->y);
    AttachPoint ap2 = findBestAttachPoint(r->e2->x, r->e2->y, r->e2->width, r->e2->height, r->x, r->y);

    // Update => replace the shared center-of-side point with a per-relationship
    // slot along that side (see get_attach_slot()/slotted_attach_point() above).
    // This is what both the line endpoints (start1_x/start2_y/etc. below) and
    // the cardinality printing further down now use, so neither collides
    // anymore when an entity has more than one relationship on the same side.
    int slot1, slot1_total;
    get_attach_slot(r->e1, ap1.side, r, &slot1, &slot1_total);
    ap1 = slotted_attach_point(r->e1, ap1.side, slot1, slot1_total);

    int slot2, slot2_total;
    get_attach_slot(r->e2, ap2.side, r, &slot2, &slot2_total);
    ap2 = slotted_attach_point(r->e2, ap2.side, slot2, slot2_total);

    int start1_x = ap1.x;
    int start1_y = ap1.y;
    int end1_x = ap_rel_e1.x;
    int end1_y = ap_rel_e1.y;
    int start2_x = ap_rel_e2.x;
    int start2_y = ap_rel_e2.y;
    int end2_x = ap2.x;
    int end2_y = ap2.y;

    switch (ap1.side) {
    case SIDE_LEFT:
        start1_x -= 1;
        break;
    case SIDE_RIGHT:
        start1_x += 1;
        break;
    case SIDE_TOP:
        start1_y -= 1;
        break;
    case SIDE_BOTTOM:
        start1_y += 1;
        break;
    }
    switch (ap_rel_e1.side) {
    case SIDE_LEFT:
        end1_x -= 1;
        break;
    case SIDE_RIGHT:
        end1_x += 1;
        break;
    case SIDE_TOP:
        end1_y -= 1;
        break;
    case SIDE_BOTTOM:
        end1_y += 1;
        break;
    }
    switch (ap_rel_e2.side) {
    case SIDE_LEFT:
        start2_x -= 1;
        break;
    case SIDE_RIGHT:
        start2_x += 1;
        break;
    case SIDE_TOP:
        start2_y -= 1;
        break;
    case SIDE_BOTTOM:
        start2_y += 1;
        break;
    }
    switch (ap2.side) {
    case SIDE_LEFT:
        end2_x -= 1;
        break;
    case SIDE_RIGHT:
        end2_x += 1;
        break;
    case SIDE_TOP:
        end2_y -= 1;
        break;
    case SIDE_BOTTOM:
        end2_y += 1;
        break;
    }

    RelGridCacheEntry *ce = rel_grid_cache_find(r);
    rel_grid_cache_touch(ce);

    AStarGrid *grid1 = NULL;
    AStarGrid *grid2 = NULL;
    bool paths_cached =
        ensure_route_grids(ce, start1_x, start1_y, end1_x, end1_y, start2_x,
                           start2_y, end2_x, end2_y, &grid1, &grid2);

    // path 1: entity1 --> relationship
    if (!paths_cached) {
        frame_asts_calls++;
        astar_free_path(ce->path1);
        ce->path1 = astar_find_path(grid1, start1_x, start1_y, end1_x, end1_y);
        if (ce->path1)
            add_endpoints_to_path(ce->path1, ap1.x, ap1.y, ap_rel_e1.x, ap_rel_e1.y);
    }
    if (ce->path1)
        draw_path_with_corners(ce->path1);

    // path 2: relationship --> entity2
    if (!paths_cached) {
        frame_asts_calls++;
        astar_free_path(ce->path2);
        ce->path2 = astar_find_path(grid2, start2_x, start2_y, end2_x, end2_y);
        if (ce->path2)
            add_endpoints_to_path(ce->path2, ap_rel_e2.x, ap_rel_e2.y, ap2.x, ap2.y);
    }
    if (ce->path2)
        draw_path_with_corners(ce->path2);

    if (r->cards[0]) {
        switch (ap1.side) {
        case SIDE_LEFT: {

            int len = (int)strlen(r->cards[0]->value);
            ap1.x -= (len + 1);
            break;
        }
        case SIDE_RIGHT:
            ap1.x += 1;
            break;
        case SIDE_TOP:
            ap1.y -= 1;
            break;
        case SIDE_BOTTOM:
            ap1.y += 1;
            break;
        }
        attron(A_BOLD);
        mvprintw(ap1.y, ap1.x, "%s", r->cards[0]->value);
        attroff(A_BOLD);
    }
    if (r->cards[1]) {
        switch (ap2.side) {
        case SIDE_LEFT: {
            int len = (int)strlen(r->cards[1]->value);
            ap2.x -= (len + 1);
            break;
        }
        case SIDE_RIGHT:
            ap2.x += 1;
            break;
        case SIDE_TOP:
            ap2.y -= 1;
            break;
        case SIDE_BOTTOM:
            ap2.y += 1;
            break;
        }
        attron(A_BOLD);
        mvprintw(ap2.y, ap2.x, "%s", r->cards[1]->value);
        attroff(A_BOLD);
    }
}

void drawConnection(Relationship *r) { drawConnectionAStar(r); }

// ==Command console==

WINDOW *create_console_window() {
    int screen_height, screen_width;
    getmaxyx(stdscr, screen_height, screen_width);

    int console_height = CONSOLE_HEIGHT;
    int console_y = screen_height - console_height;

    WINDOW *console_win = newwin(console_height, screen_width, console_y, 0);
    box(console_win, 0, 0);
    mvwprintw(console_win, 0, 2, " Console ");
    return console_win;
}

WINDOW *create_help_window() {
    int std_screen_width, std_screen_height;
    getmaxyx(stdscr, std_screen_height, std_screen_width);

    int h = HELP_WIN_HEIGHT;
    int w = HELP_WIN_WIDTH;
    if (h >= std_screen_height - 2)
        h = std_screen_height - 2;
    if (w >= std_screen_width - 2)
        w = std_screen_width - 2;

    int y = (std_screen_height - h) / 2;
    int x = (std_screen_width - w) / 2;
    WINDOW *win = newwin(h, w, y, x);
    leaveok(win, TRUE);
    return win;
}

void draw_console_prompt(WINDOW *console_win, const char *input, status status) {
    int win_height = getmaxy(console_win);
    int win_width = getmaxx(console_win);

    int std_screen_width, std_screen_height;
    getmaxyx(stdscr, std_screen_height, std_screen_width);

    if (status == Typing || status == Editing) {
        wattron(console_win, COLOR_PAIR(0));
        wmove(console_win, win_height - 2, 1);
        wclrtoeol(console_win);

        box(console_win, 0, 0);

        // header
        wattron(console_win, A_BOLD);
        mvwprintw(console_win, 0, 2, " Console ");
        wattroff(console_win, A_BOLD);

        // status badge
        if (status == Typing) {
            wattron(console_win, COLOR_PAIR(7) | A_BOLD | A_REVERSE);
            mvwprintw(console_win, 0, win_width - 10, " TYPING ");
            wattroff(console_win, COLOR_PAIR(7) | A_BOLD | A_REVERSE);
        } else if (status == Editing) {
            wattron(console_win, COLOR_PAIR(4) | A_BOLD | A_REVERSE);
            mvwprintw(console_win, 0, win_width - 11, " EDITING ");
            wattroff(console_win, COLOR_PAIR(4) | A_BOLD | A_REVERSE);
        }

        wattron(console_win, COLOR_PAIR(7) | A_BOLD);
        mvwprintw(console_win, win_height - 2, 1, ">> ");
        wattroff(console_win, COLOR_PAIR(7) | A_BOLD);

        wattron(console_win, A_BOLD);
        wprintw(console_win, "%s", input);
        wattroff(console_win, A_BOLD);

        // UPDATE => wnoutrefresh+doupdate instead of wrefresh so console never
        //           triggers a mid-frame physical write while help is composing.
        //           doupdate must live here — if it's outside in main.c it only
        //           fires after the next getch() unblocks, causing 1-frame lag.
        wnoutrefresh(console_win);
        doupdate();
    }
}

void draw_help_window(WINDOW *win, HelpWindow *hwin, const char *search_buffer, HelpPage page, HelpAction Action,
                      WINDOW *scrolling_pad) {
    int std_screen_width, std_screen_height;
    getmaxyx(stdscr, std_screen_height, std_screen_width);

    int h = HELP_WIN_HEIGHT;
    int w = HELP_WIN_WIDTH;

    if (h >= std_screen_height - 2)
        h = std_screen_height - 2;
    if (w >= std_screen_width - 2)
        w = std_screen_width - 2;

    int help_win_y = (std_screen_height - h) / 2;
    int help_win_x = (std_screen_width - w) / 2;

    // wresize+mvwin removed from per-frame draw; create_help_window
    //  already positions the window correctly. Calling mvwin every frame
    //  forces a full terminal repaint on Wayland which is the flicker.
    //  Re-add here only if you implement terminal resize handling (SIGWINCH).

    werase(win);
    box(win, 0, 0);

    // Header
    wattron(win, COLOR_PAIR(6) | A_BOLD | A_REVERSE);
    mvwprintw(win, 1, (w / 2) - 4, "  HELP  ");
    wattroff(win, COLOR_PAIR(6) | A_BOLD | A_REVERSE);

    // Pad coords
    int sminrow = help_win_y + 2;     // start rendering below top border and header
    int smincol = help_win_x + 2;     // leave a 1 char margin inside left border
    int smaxcol = help_win_x + w - 3; // stop a 1 char margin inside right border

    int smaxrow = help_win_y + h - 2; // default bottom limit

    if (Action) {
        wmove(win, h - 2, 1);
        wclrtoeol(win);
        wmove(win, h - 3, 1);
        whline(win, ACS_HLINE, w - 2); // w-2 so it doesn't break the vertical borders
        mvwprintw(win, h - 2, 1, "/%s", search_buffer);

        // shrinking the pad's rendering area so it doesn't overwrite the search bar
        smaxrow = help_win_y + h - 4;
    }

    wnoutrefresh(win);
    //  touchwin(win);
    //  force ncurses to not-optimise and draw the whole window (didn't work)

    switch (page) {
    case Main:
        pnoutrefresh(scrolling_pad, hwin->main_scrolling_line, 0, sminrow, smincol, smaxrow, smaxcol);
        break;
    case Hotkeys:
        pnoutrefresh(scrolling_pad, PAD_HOTKEYS_OFFSET + hwin->hotkey_scrolling_line, 0, sminrow, smincol, smaxrow,
                     smaxcol);
        break;
    case Examples:
        pnoutrefresh(scrolling_pad, PAD_EXAMPLES_OFFSET + hwin->examples_scrolling_line, 0, sminrow, smincol, smaxrow,
                     smaxcol);
        break;
    }
    doupdate();
    // mvwprintw(win, smaxrow - 2, (w / 2) - 16, "  Press 'q' to quit  ");
    // wrefresh(win);
}

void draw_all_entities(const GlobalObjects *global_objects, int moving_index, bool is_moving) {
    for (int i = 0; i < global_objects->entity_count; i++) {
        if (global_objects->entities[i]) {
            if (is_moving && i == moving_index) {
                attron(COLOR_PAIR(3));
                drawEntity(global_objects->entities[i]);
                attroff(COLOR_PAIR(3));
            } else {
                attron(COLOR_PAIR(7));
                drawEntity(global_objects->entities[i]);
                attroff(COLOR_PAIR(7));
            }
        }
    }
}

// Free cached grids for relationships that no longer exist.
static void rel_grid_cache_prune(void) {
    for (int i = 0; i < REL_GRID_CACHE_SIZE; i++) {
        RelGridCacheEntry *e = &rel_grid_cache[i];
        if (!e->in_use)
            continue;

        bool alive = false;
        for (int j = 0; j < global_objects.relationship_count; j++) {
            if (global_objects.relationships[j] == e->rel) {
                alive = true;
                break;
            }
        }
        if (!alive) {
            astar_free_grid(e->grid1);
            astar_free_grid(e->grid2);
            astar_free_path(e->path1);
            astar_free_path(e->path2);
            e->grid1 = NULL;
            e->grid2 = NULL;
            e->path1 = NULL;
            e->path2 = NULL;
            e->in_use = false;
        }
    }
}

void draw_all_relationships(const GlobalObjects *global_objects, int moving_index, bool is_moving) {
    rel_grid_cache_prune();
    detect_changes();
    for (int i = 0; i < global_objects->relationship_count; i++) {
        if (global_objects->relationships[i]) {
            if (is_moving && moving_index == i) {
                attron(COLOR_PAIR(2));
                drawRelationship(global_objects->relationships[i]);
                drawConnection(global_objects->relationships[i]);
                attroff(COLOR_PAIR(2));
            } else {
                attron(COLOR_PAIR(5));
                drawRelationship(global_objects->relationships[i]);
                drawConnection(global_objects->relationships[i]);
                attroff(COLOR_PAIR(5));
            }
        }
    }
}

static bool frame_stats_overlay = false;
static void draw_frame_stats_overlay(void);

void draw_all_and_refresh(int screen_width, int moving_index, bool is_moving,
                          bool *needs_redraw) {
    // Frame timing
    struct timespec t0, t1, t2, t3;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    erase();
    clock_gettime(CLOCK_MONOTONIC, &t1);
    frame_asts_calls = 0;
    draw_all_entities(&global_objects, moving_index, is_moving);
    clock_gettime(CLOCK_MONOTONIC, &t2);
    draw_all_relationships(&global_objects, moving_index, is_moving);
    clock_gettime(CLOCK_MONOTONIC, &t3);

    frame_erase_us = (t1.tv_sec - t0.tv_sec) * 1000000L + (t1.tv_nsec - t0.tv_nsec) / 1000L;
    frame_entities_us = (t2.tv_sec - t1.tv_sec) * 1000000L + (t2.tv_nsec - t1.tv_nsec) / 1000L;
    frame_relationships_us = (t3.tv_sec - t2.tv_sec) * 1000000L + (t3.tv_nsec - t2.tv_nsec) / 1000L;
    last_frame_us = (t3.tv_sec - t0.tv_sec) * 1000000L + (t3.tv_nsec - t0.tv_nsec) / 1000L;

    if (frame_stats_overlay)
        draw_frame_stats_overlay();

    // queue stdscr and then console_win's doupdate inside draw_console_prompt
    // will flush both together when called right after this in the main loop
    wnoutrefresh(stdscr);

    // Testing hook: run with MCD_PERF=1 to log per-frame draw timing + cache
    // hit/miss stats to perf.log.
    static int perf_enabled = -1;
    if (perf_enabled < 0)
        perf_enabled = (getenv("MCD_PERF") != NULL);
    if (perf_enabled)
        print_frame_stats();

    *needs_redraw = false;
}

void toggle_frame_stats(void) {
    frame_stats_overlay = !frame_stats_overlay;
    if (frame_stats_overlay) {
        astar_cache_reset_stats();
        grid_rebuilds = 0;
        grid_reuses = 0;
    }
}

// On-screen overlay (top-left) showing frame time, FPS and A* cache hit/miss
// counts.  Enabled via the "perf" command.
static void draw_frame_stats_overlay(void) {
    unsigned long hits = 0, misses = 0;
    astar_cache_stats(&hits, &misses);

    // Rolling FPS: count frames over a 1-second window.
    static struct timespec fps_t0 = {0};
    static long fps_frames = 0;
    static double fps = 0.0;
    if (fps_frames == 0) {
        clock_gettime(CLOCK_MONOTONIC, &fps_t0);
    }
    fps_frames++;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double elapsed = (now.tv_sec - fps_t0.tv_sec) +
                     (now.tv_nsec - fps_t0.tv_nsec) / 1e9;
    if (elapsed >= 1.0) {
        fps = fps_frames / elapsed;
        fps_frames = 0;
    }

    double hit_pct = (hits + misses) ? 100.0 * hits / (hits + misses) : 0.0;
    attron(COLOR_PAIR(3));
    mvprintw(0, 0, "frame: %6ldus (erase %ld + ent %ld + rel %ld) | fps:%5.1f | astar:%ld hit:%lu miss:%lu (%.0f%%) | rebuild:%lu reuse:%lu",
             last_frame_us, frame_erase_us, frame_entities_us, frame_relationships_us,
             fps, frame_asts_calls, hits, misses, hit_pct, grid_rebuilds, grid_reuses);
    attroff(COLOR_PAIR(3));
}

// Testing helper: appends one line per call with the time the last frame took
// to draw, plus A* cache hit/miss counters and how many route grids were
// rebuilt vs reused.  Written to perf.log so it can't corrupt the ncurses
// screen; safe to call from the redraw loop.
void print_frame_stats(void) {
    unsigned long hits = 0, misses = 0;
    astar_cache_stats(&hits, &misses);

    static FILE *log = NULL;
    if (!log) {
        log = fopen("perf.log", "w");
        if (log)
            fprintf(log, "frame_us astar_hits astar_misses hit_pct grid_rebuilds grid_reuses\n");
    }
    if (!log)
        return;

    double hit_pct = (hits + misses) ? 100.0 * hits / (hits + misses) : 0.0;
    fprintf(log, "%ld %lu %lu %.1f %lu %lu\n", last_frame_us, hits, misses,
            hit_pct, grid_rebuilds, grid_reuses);
    fflush(log);
}

WINDOW *init_pad(int num_lines, int num_col, HelpWindow hwin) {
    WINDOW *pad = newpad(num_lines, num_col);
    if (!pad)
        return NULL;

    // write all lines to the pad

    for (int p = 0; p < HelpPageNum; p++) {
        for (size_t l = 0; l < hwin.pages_db[p].line_count; l++) {
            HelpLine *current_line = (HelpLine *)&hwin.pages_db[p].lines[l];
            if (current_line->text != NULL) {
                mvwprintw(pad, current_line->line_start, 2, "%s", current_line->text);
            }
        }
    }

    return pad;
}

void revert_back_to_console(WINDOW *console_win, status *status, bool *needs_redraw) {
    if (!console_win) {
        return;
    }

    *status = Typing;
    last_status = Typing;
    int screen_height, screen_width;
    getmaxyx(stdscr, screen_height, screen_width);

    int console_y = screen_height - CONSOLE_HEIGHT;

    wresize(console_win, CONSOLE_HEIGHT, screen_width);
    mvwin(console_win, console_y, 0);
    werase(console_win);
    curs_set(1); // restore cursor

    *needs_redraw = true;
}

// TODO: implement a graphical logging funciton that even
// writes on stdscr
void search_help(WINDOW *win, HelpWindow *hwin, char search_buffer[], int search_len, HelpAction *action, HelpPage page,
                 WINDOW *scrolling_pad) {
    *action = Search;
    while (*action) {
        draw_help_window(win, hwin, search_buffer, page, *action, scrolling_pad);

        int search_char = getch();
        if (search_char == ERR) {
            continue;
        }

        if (search_char == '\n') {
            if (search_len == 0) {
                *action = Navigation;
                search_len = 0;
                search_buffer[0] = '\0';
                return;
            }
            SearchResult *matches = search_help_kmp(hwin, search_buffer, search_len);
            if (matches) {
                highlight_search_matches(hwin, win, matches, search_buffer);

                HelpPage curr = hwin->current_page;
                int curr_page_first_line = hwin->pages_db[curr].lines[0].line_start;

                int num_matches = vec_len(matches);

                bool searching = true;
                int curr_line_idx = 0;

                while (searching) {
                    int hop = getch();
                    if (hop == ERR)
                        continue;

                    switch (hop) {
                    case 'n': // forward

                        /* Notes on absolute and relative positions
                         *  compare index to the length of the matches array
                         *  and Convert absolute line to relative scrolling line
                         * int relative_line = matches[curr_line_idx].line - curr_page_first_line + 1;
                         */

                        if (curr_line_idx + 1 < num_matches) {
                            ++curr_line_idx;
                            int relative_line = matches[curr_line_idx].line - curr_page_first_line + 1;
                            set_scrolling_line(hwin, relative_line);
                        }
                        break;
                    case 'N': // backward
                        if (curr_line_idx > 0) {
                            --curr_line_idx;
                            int relative_line = matches[curr_line_idx].line - curr_page_first_line + 1;
                            set_scrolling_line(hwin, relative_line);
                        }
                        break;
                    case 'q':
                        searching = false;
                        break;
                    }
                    wnoutrefresh(win);
                    doupdate();
                }
                destroy_search_results(matches);
            }

            search_len = 0;
            search_buffer[0] = '\0';
        } else if (search_char == KEY_ESCAPE) { // escape
            *action = Navigation;
            search_len = 0;
            search_buffer[0] = '\0';
        } else if (search_char == KEY_BACKSPACE || search_char == 127) {
            if (search_len > 0) {
                search_buffer[--search_len] = '\0';
            }
        } else if (search_char >= 32 && search_char <= 126) {
            if (search_len < 255) {
                search_buffer[search_len++] = search_char;
                search_buffer[search_len] = '\0';
            }
        }
    }
}

void highlight_search_matches(HelpWindow *hwin, WINDOW *win, SearchResult *matches, const char *search_buffer) {
    HelpPage curr_page = hwin->current_page;
    int offset;

    switch (curr_page) {
    case Main:
        offset = hwin->main_scrolling_line;
        break;
    case Hotkeys:
        offset = hwin->hotkey_scrolling_line;
        break;
    case Examples:
        offset = hwin->examples_scrolling_line;
        break;
    }

    //  get the absolute starting line of the current page to  normalize the coordinates
    int page_start_line = 1;
    if (hwin->pages_db && hwin->pages_db[curr_page].line_count > 0) {
        page_start_line = hwin->pages_db[curr_page].lines[0].line_start;
    }

    int std_screen_width, std_screen_height;
    getmaxyx(stdscr, std_screen_height, std_screen_width);

    int h = HELP_WIN_HEIGHT;
    if (h >= std_screen_height - 2)
        h = std_screen_height - 2;

    int num_matches = vec_len(matches);
    int total_num_matches = 0;

    for (size_t i = 0; i < num_matches; i++) {
        int line = matches[i].line;
        int *line_matches = matches[i].idx;

        if (line_matches) {
            int line_matches_num = vec_len(line_matches);
            for (int j = 0; j < line_matches_num; j++) {
                wattron(win, COLOR_PAIR(6) | A_BOLD | A_REVERSE);

                // normalize the absolute line number to a relative row in the viewport
                //  (absolute_line - page_start) - offset + viewport_padding
                int target_row = (line - page_start_line) - offset + 3;

                if (target_row >= 2 && target_row < h - 1) {
                    mvwprintw(win, target_row, line_matches[j] + 4, "%s", search_buffer);
                }
                wattroff(win, COLOR_PAIR(6) | A_BOLD | A_REVERSE);
                total_num_matches++;
            }
        }
    }
    wnoutrefresh(win);
    doupdate();
}

// === Utility Functions ===

void clear_console_log(WINDOW *console_win) {

    wmove(console_win, 1, 1);
    wclrtoeol(console_win);

    wmove(console_win, 2, 1);
    wclrtoeol(console_win);

    wmove(console_win, 3, 1);
    wclrtoeol(console_win);
}

// Debugging Functions (to be removed)

WINDOW *create_ast_debug_window() {
    int screen_height, screen_width;
    getmaxyx(stdscr, screen_height, screen_width);

    // Give it a reasonable height, e.g., 1/3rd of the screen, or max 50
    int debug_height = (screen_height / 3 < 30) ? (screen_height / 3) : 30;
    int start_y = 2; // Start near the top instead of 50
    int start_x = 2;

    WINDOW *debug_window = newwin(debug_height, screen_width / 2, start_y, start_x);

    if (debug_window != NULL) {
        box(debug_window, 0, 0);
        mvwprintw(debug_window, 0, 2, " Console ");
    } else {
        // Handle the error gracefully, maybe log it
    }

    return debug_window;
}
void draw_ast_debug_window(WINDOW *debug_window, AST *tree) {
    if (!debug_window || !tree)
        return;

    int win_height = getmaxy(debug_window);
    werase(debug_window);
    box(debug_window, 0, 0);

    // Draw header inside local window space
    wattron(debug_window, COLOR_PAIR(7) | A_BOLD);
    mvwprintw(debug_window, 1, 1, "DEBUGGING AST:");
    wattroff(debug_window, COLOR_PAIR(7) | A_BOLD);

    ASTNode *temp = tree->head;
    int local_y = 2; // Start printing content on line 2 (below the header)

    // Stay within window boundaries (leave room for bottom border)
    while (temp && local_y < win_height - 1) {
        wmove(debug_window, local_y, 1);
        wclrtoeol(debug_window);

        if (temp->cmd->type == ADD) {
            mvwprintw(debug_window, local_y, 2, "ADD: %s . %s (%s)", temp->cmd->cmds.add_command.identifier_name,
                      temp->cmd->cmds.add_command.Data.p.prop_name, temp->cmd->cmds.add_command.Data.p.prop_type);
        } else if (temp->cmd->type == CREATE) {
            if (temp->cmd->cmds.create_command.type == TYPE_ENTITY) {
                mvwprintw(debug_window, local_y, 2, "CR_ENT: %s", temp->cmd->cmds.create_command.Data.e.name);
            } else if (temp->cmd->cmds.create_command.type == TYPE_RELATIONSHIP) {
                mvwprintw(debug_window, local_y, 2, "CR_REL: %s", temp->cmd->cmds.create_command.Data.r.name);
            }
        } else if (temp->cmd->type == CONVERT) {
            DiagramType curr = temp->cmd->cmds.convert_command.type;
            if (curr == MCD) {
                mvwprintw(debug_window, local_y, 2, "CONVERT: MCD");
            } else if (curr == MLD) {
                mvwprintw(debug_window, local_y, 2, "CONVERT: MLD");
            } else if (curr == SQL) {
                mvwprintw(debug_window, local_y, 2, "CONVERT: SQL");
            }
        } else if (temp->cmd->type == CHANGE_NAME) {
            mvwprintw(debug_window, local_y, 2, "CHANGE_NAME: %s -> %s", temp->cmd->cmds.change_name_command.old_name,
                      temp->cmd->cmds.change_name_command.new_name);
        } else if (temp->cmd->type == SAVE) {
            const char *dtype = (temp->cmd->cmds.save_command.diagram_type == MLD) ? "MLD" : "MCD";
            mvwprintw(debug_window, local_y, 2, "SAVE: %s ->  %s ", dtype, temp->cmd->cmds.save_command.filename);
        }

        local_y++;
        temp = temp->next;
    }

    wnoutrefresh(debug_window);
}
