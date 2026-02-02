#include "graphics.h"
#include "DSA/astar.h"
#include "DSA/pqueue.h"
#include <string.h>

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

AStarPath *smooth_path(AStarPath *path) {
    if (!path || path->length <= 2) {
        AStarPath *new_path = malloc(sizeof(AStarPath));
        new_path->length = path->length;
        new_path->path_x = malloc(new_path->length * sizeof(int));
        new_path->path_y = malloc(new_path->length * sizeof(int));
        for (int i = 0; i < new_path->length; i++) {
            new_path->path_x[i] = path->path_x[i];
            new_path->path_y[i] = path->path_y[i];
        }
        return new_path;
    }

    AStarPath *smooth = malloc(sizeof(AStarPath));
    smooth->path_x = malloc(path->length * sizeof(int));
    smooth->path_y = malloc(path->length * sizeof(int));
    smooth->length = 0;

    smooth->path_x[0] = path->path_x[0];
    smooth->path_y[0] = path->path_y[0];
    smooth->length = 1;

    for (int i = 1; i < path->length - 1; i++) {
        int prev_x = path->path_x[i - 1];
        int prev_y = path->path_y[i - 1];
        int curr_x = path->path_x[i];
        int curr_y = path->path_y[i];
        int next_x = path->path_x[i + 1];
        int next_y = path->path_y[i + 1];

        int dir1_x = curr_x - prev_x;
        int dir1_y = curr_y - prev_y;
        int dir2_x = next_x - curr_x;
        int dir2_y = next_y - curr_y;

        if (dir1_x != dir2_x || dir1_y != dir2_y) {
            smooth->path_x[smooth->length] = curr_x;
            smooth->path_y[smooth->length] = curr_y;
            smooth->length++;
        }
    }

    smooth->path_x[smooth->length] = path->path_x[path->length - 1];
    smooth->path_y[smooth->length] = path->path_y[path->length - 1];
    smooth->length++;

    smooth->path_x = realloc(smooth->path_x, smooth->length * sizeof(int));
    smooth->path_y = realloc(smooth->path_y, smooth->length * sizeof(int));

    return smooth;
}

void draw_path_with_corners(AStarPath *path) {
    if (!path || path->length < 2)
        return;

    // Draw debug markers at each path point
    for (int i = 0; i < path->length; i++) {
        mvaddch(path->path_y[i], path->path_x[i], 'X');
    }
    refresh();
    getch(); // Press any key to continue

    attron(COLOR_PAIR(3));
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
        int prev_x = path->path_x[i - 1];
        int prev_y = path->path_y[i - 1];
        int curr_x = path->path_x[i];
        int curr_y = path->path_y[i];
        int next_x = path->path_x[i + 1];
        int next_y = path->path_y[i + 1];

        int from_x = curr_x - prev_x;
        int from_y = curr_y - prev_y;
        int to_x = next_x - curr_x;
        int to_y = next_y - curr_y;

        if (from_x > 0 && to_y > 0) {
            mvaddch(curr_y, curr_x, ACS_LRCORNER);
        } else if (from_x > 0 && to_y < 0) {
            mvaddch(curr_y, curr_x, ACS_LRCORNER);
        } else if (from_x < 0 && to_y > 0) {
            mvaddch(curr_y, curr_x, ACS_URCORNER);
        } else if (from_x < 0 && to_y < 0) {
            mvaddch(curr_y, curr_x, ACS_HLINE);
        } else if (from_y > 0 && to_x > 0) {
            mvaddch(curr_y, curr_x, ACS_LLCORNER);
        } else if (from_y > 0 && to_x < 0) {
            mvaddch(curr_y, curr_x, ACS_URCORNER);
        } else if (from_y < 0 && to_x > 0) {
            mvaddch(curr_y, curr_x, ACS_LRCORNER);
        } else if (from_y < 0 && to_x < 0) {
            mvaddch(curr_y, curr_x, ACS_LRCORNER);
        }
    }

    attroff(COLOR_PAIR(3));
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
        start1_x--;
        break;
    case SIDE_RIGHT:
        start1_x++;
        break;
    case SIDE_TOP:
        start1_y--;
        break;
    case SIDE_BOTTOM:
        start1_y++;
        break;
    }

    switch (ap_rel_e1.side) {
    case SIDE_LEFT:
        end1_x--;
        break;
    case SIDE_RIGHT:
        end1_x++;
        break;
    case SIDE_TOP:
        end1_y--;
        break;
    case SIDE_BOTTOM:
        end1_y++;
        break;
    }

    switch (ap_rel_e2.side) {
    case SIDE_LEFT:
        start2_x--;
        break;
    case SIDE_RIGHT:
        start2_x++;
        break;
    case SIDE_TOP:
        start2_y--;
        break;
    case SIDE_BOTTOM:
        start2_y++;
        break;
    }

    switch (ap2.side) {
    case SIDE_LEFT:
        end2_x--;
        break;
    case SIDE_RIGHT:
        end2_x++;
        break;
    case SIDE_TOP:
        end2_y--;
        break;
    case SIDE_BOTTOM:
        end2_y++;
        break;
    }

    int margin = 10;

    AStarGrid *grid1 =
        astar_create_grid(start1_x, start1_y, end1_x, end1_y, margin);

    for (int i = 0; i < global_objects.entity_count; i++) {
        Entity *e = global_objects.entities[i];
        if (e) {
            astar_mark_obstacle(grid1, e->x, e->y, e->width, e->height);
        }
    }

    for (int i = 0; i < global_objects.relationship_count; i++) {
        Relationship *rel = global_objects.relationships[i];
        if (rel) {
            astar_mark_obstacle(grid1, rel->x, rel->y, rel->width, rel->height);
        }
    }

    AStarPath *path1 =
        astar_find_path(grid1, start1_x, start1_y, end1_x, end1_y);

    AStarGrid *grid2 =
        astar_create_grid(start2_x, start2_y, end2_x, end2_y, margin);

    for (int i = 0; i < global_objects.entity_count; i++) {
        Entity *e = global_objects.entities[i];
        if (e) {
            astar_mark_obstacle(grid2, e->x, e->y, e->width, e->height);
        }
    }

    for (int i = 0; i < global_objects.relationship_count; i++) {
        Relationship *rel = global_objects.relationships[i];
        if (rel) {
            astar_mark_obstacle(grid2, rel->x, rel->y, rel->width, rel->height);
        }
    }

    AStarPath *path2 =
        astar_find_path(grid2, start2_x, start2_y, end2_x, end2_y);

    if (path1) {
        AStarPath *smooth1 = smooth_path(path1);
        draw_path_with_corners(smooth1);
        astar_free_path(smooth1);
        astar_free_path(path1);
    }

    if (path2) {
        AStarPath *smooth2 = smooth_path(path2);
        draw_path_with_corners(smooth2);
        astar_free_path(smooth2);
        astar_free_path(path2);
    }

    if (r->cards[0]) {
        int card_x = ap1.x;
        int card_y = ap1.y;

        switch (ap1.side) {
        case SIDE_TOP:
            card_y -= 1;
            card_x -= 2;
            break;
        case SIDE_BOTTOM:
            card_y += 1;
            card_x -= 2;
            break;
        case SIDE_LEFT:
            card_x -= 5;
            break;
        case SIDE_RIGHT:
            card_x += 2;
            break;
        }

        mvprintw(card_y, card_x, "%s", r->cards[0]->value);
    }

    if (r->cards[1]) {
        int card_x = ap2.x;
        int card_y = ap2.y;

        switch (ap2.side) {
        case SIDE_TOP:
            card_y -= 1;
            card_x -= 2;
            break;
        case SIDE_BOTTOM:
            card_y += 1;
            card_x -= 2;
            break;
        case SIDE_LEFT:
            card_x -= 5;
            break;
        case SIDE_RIGHT:
            card_x += 2;
            break;
        }

        mvprintw(card_y, card_x, "%s", r->cards[1]->value);
    }

    astar_free_grid(grid1);
    astar_free_grid(grid2);
}

void drawConnection(Relationship *r) { drawConnectionAStar(r); }
