#include "graphics.h"
#include "DSA/astar.h"
#include "DSA/pqueue.h"
#include <string.h>
// Add this function to main.c for debugging
void debugPrintPath(AStarPath *path, const char *name) {
    if (!path) {
        mvprintw(0, 0, "%s: No path found", name);
    } else {
        mvprintw(0, 0, "%s: Path length %d", name, path->length);
        for (int i = 0; i < path->length; i++) {
            mvprintw(1 + i, 0, "  [%d] = (%d, %d)", i, path->path_x[i],
                     path->path_y[i]);
        }
    }
    refresh();
    getch();
}
void initColors() {
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
}

void draw_hline_at(int y, int x1, int x2, chtype ch) {
    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    for (int x = x1; x <= x2; x++) {
        mvaddch(y, x, ch);
    }
}

void draw_vline_at(int x, int y1, int y2, chtype ch) {
    if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
    }
    for (int y = y1; y <= y2; y++) {
        mvaddch(y, x, ch);
    }
}

void drawEntity(Entity *e) {
    attron(COLOR_PAIR(1));

    mvaddch(e->y, e->x, ACS_ULCORNER);
    for (int i = 1; i < e->width - 1; i++) {
        mvaddch(e->y, e->x + i, ACS_HLINE);
    }
    mvaddch(e->y, e->x + e->width - 1, ACS_URCORNER);

    for (int i = 1; i < e->height - 1; i++) {
        mvaddch(e->y + i, e->x, ACS_VLINE);
        mvprintw(e->y + i, e->x + 1, "%-*s", e->width - 2, "");
        mvaddch(e->y + i, e->x + e->width - 1, ACS_VLINE);
    }

    mvaddch(e->y + e->height - 1, e->x, ACS_LLCORNER);
    for (int i = 1; i < e->width - 1; i++) {
        mvaddch(e->y + e->height - 1, e->x + i, ACS_HLINE);
    }
    mvaddch(e->y + e->height - 1, e->x + e->width - 1, ACS_LRCORNER);

    mvprintw(e->y + 1, e->x + (e->width - strlen(e->name)) / 2, "%s", e->name);

    for (int i = 0; i < e->num_properties; i++) {
        if (e->properties[i]) {
            char prop_str[64];
            snprintf(prop_str, sizeof(prop_str), "%s:%s",
                     e->properties[i]->name, e->properties[i]->type);
            mvprintw(e->y + 2 + i, e->x + 1, "%-*s", e->width - 2, prop_str);
        }
    }

    attroff(COLOR_PAIR(1));
}

void drawRelationship(Relationship *r) {
    attron(COLOR_PAIR(2));

    mvaddch(r->y, r->x, ACS_ULCORNER);
    for (int i = 1; i < r->width - 1; i++) {
        mvaddch(r->y, r->x + i, ACS_HLINE);
    }
    mvaddch(r->y, r->x + r->width - 1, ACS_URCORNER);

    for (int i = 1; i < r->height - 1; i++) {
        mvaddch(r->y + i, r->x, ACS_VLINE);
        mvprintw(r->y + i, r->x + 1, "%-*s", r->width - 2, "");
        mvaddch(r->y + i, r->x + r->width - 1, ACS_VLINE);
    }

    mvaddch(r->y + r->height - 1, r->x, ACS_LLCORNER);
    for (int i = 1; i < r->width - 1; i++) {
        mvaddch(r->y + r->height - 1, r->x + i, ACS_HLINE);
    }
    mvaddch(r->y + r->height - 1, r->x + r->width - 1, ACS_LRCORNER);

    mvprintw(r->y + 1, r->x + (r->width - strlen(r->name)) / 2, "%s", r->name);

    for (int i = 0; i < r->num_properties; i++) {
        if (r->properties[i]) {
            char prop_str[64];
            snprintf(prop_str, sizeof(prop_str), "%s:%s",
                     r->properties[i]->name, r->properties[i]->type);
            mvprintw(r->y + 2 + i, r->x + 1, "%-*s", r->width - 2, prop_str);
        }
    }

    attroff(COLOR_PAIR(2));
}

void drawAStarPath(AStarPath *path) {
    if (!path || path->length < 2)
        return;

    attron(COLOR_PAIR(6) | A_BOLD);

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

    for (int i = 1; i < path->length - 1; i++) {
        int x = path->path_x[i];
        int y = path->path_y[i];
        int prev_x = path->path_x[i - 1];
        int prev_y = path->path_y[i - 1];
        int next_x = path->path_x[i + 1];
        int next_y = path->path_y[i + 1];

        if ((prev_x == x && next_y == y) || (prev_y == y && next_x == x)) {
            if (prev_y < y && next_x > x) {
                mvaddch(y, x, ACS_ULCORNER);
            } else if (prev_y < y && next_x < x) {
                mvaddch(y, x, ACS_LLCORNER);
            } else if (prev_y > y && next_x > x) {
                mvaddch(y, x, ACS_ULCORNER);
            } else if (prev_y > y && next_x < x) {
                mvaddch(y, x, ACS_LRCORNER);
            }
        }
    }

    attroff(COLOR_PAIR(6) | A_BOLD);
}

void drawConnectionAStar(Relationship *r) {
    if (!r || !r->e1 || !r->e2)
        return;

    AttachPoint ap1 = findBestAttachPoint(r->e1->x, r->e1->y, r->e1->width,
                                          r->e1->height, r->x, r->y);
    AttachPoint ap_rel_e1 = findBestAttachPoint(r->x, r->y, r->width, r->height,
                                                r->e1->x, r->e1->y);
    AttachPoint ap_rel_e2 = findBestAttachPoint(r->x, r->y, r->width, r->height,
                                                r->e2->x, r->e2->y);
    AttachPoint ap2 = findBestAttachPoint(r->e2->x, r->e2->y, r->e2->width,
                                          r->e2->height, r->x, r->y);

    int margin = 8;

    // ============================================================
    // Path 1: Entity1 -> Relationship
    // ============================================================
    AStarGrid *grid1 =
        astar_create_grid(ap1.x, ap1.y, ap_rel_e1.x, ap_rel_e1.y, margin);

    // Mark obstacles - DON'T mark e1 or r (we're connecting them!)
    for (int i = 0; i < global_objects.entity_count; i++) {
        Entity *e = global_objects.entities[i];
        if (e && e != r->e1) { // ← DON'T mark Entity1!
            astar_mark_obstacle(grid1, e->x, e->y, e->width, e->height);
        }
    }

    for (int i = 0; i < global_objects.relationship_count; i++) {
        Relationship *rel = global_objects.relationships[i];
        if (rel && rel != r) { // ← DON'T mark current relationship!
            astar_mark_obstacle(grid1, rel->x, rel->y, rel->width, rel->height);
        }
    }

    AStarPath *path1 =
        astar_find_path(grid1, ap1.x, ap1.y, ap_rel_e1.x, ap_rel_e1.y);

    // debugPrintPath(path1, "Path E1->R");

    // ============================================================
    // Path 2: Relationship -> Entity2
    // ============================================================
    AStarGrid *grid2 =
        astar_create_grid(ap_rel_e2.x, ap_rel_e2.y, ap2.x, ap2.y, margin);

    // Mark obstacles - DON'T mark r or e2 (we're connecting them!)
    for (int i = 0; i < global_objects.entity_count; i++) {
        Entity *e = global_objects.entities[i];
        if (e && e != r->e2) { // ← DON'T mark Entity2!
            astar_mark_obstacle(grid2, e->x, e->y, e->width, e->height);
        }
    }

    for (int i = 0; i < global_objects.relationship_count; i++) {
        Relationship *rel = global_objects.relationships[i];
        if (rel && rel != r) { // ← DON'T mark current relationship!
            astar_mark_obstacle(grid2, rel->x, rel->y, rel->width, rel->height);
        }
    }

    AStarPath *path2 =
        astar_find_path(grid2, ap_rel_e2.x, ap_rel_e2.y, ap2.x, ap2.y);

    // debugPrintPath(path2, "Path R->E2");

    // Draw the paths
    if (path1) {
        drawAStarPath(path1);
        astar_free_path(path1);
    }

    if (path2) {
        drawAStarPath(path2);
        astar_free_path(path2);
    }

    // Draw cardinalities
    if (r->cards[0]) {
        mvprintw(ap1.y, ap1.x - 4, "%s", r->cards[0]->value);
    }

    if (r->cards[1]) {
        mvprintw(ap2.y, ap2.x - 4, "%s", r->cards[1]->value);
    }

    astar_free_grid(grid1);
    astar_free_grid(grid2);
}

void drawConnection(Relationship *r) { drawConnectionAStar(r); }
