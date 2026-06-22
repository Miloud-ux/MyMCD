// == CHANGES : ==
// - draw_ast_debug_window(): added an "else if (temp->cmd->type == CHANGE_NAME)"
//   branch after the CONVERT branch so the new command type renders in the
//   AST debug window instead of being silently skipped.
// - drawEntity(): fixed the FOREIGN_KEY rendering branch (the "*" prefix). The
//   ":type" suffix was always printed at "e->x + 1 + strlen(name)", which is
//   where the name would start WITHOUT the "*" - but the FK name is printed one
//   column over (at "e->x + 2") to make room for the "*". So the ":type" print
//   landed one column too far left and overwrote the property name's last
//   character. Introduced a name_col variable that tracks where the name was
//   actually printed (e->x + 1, or e->x + 2 for FK) and used it for both prints.
#include "graphics.h"
#include "DSA/astar.h"
#include "DSA/kmp.h"
#include "DSA/vec.h"
#include "help_window.h"
#include <ncurses.h>
#include <string.h>
#include <time.h>

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
    } else if (e->y < 0) {
        e->y = 0;
    } else if (e->y + e->height + CONSOLE_HEIGHT > max_scr_y) {
        int dist = (e->y + e->height + CONSOLE_HEIGHT) - max_scr_y;
        e->y -= dist;
    }

    // == Old implementation kept for more research ===
    // for (int i = 1; i < e->height - 1; i++) {
    //     mvprintw(e->y + i, e->x + 1, "%-*s", e->width - 2, "");
    //     // why is this -2 ?  at least -1 ? does this affect A* ?
    //     mvaddch(e->y + i, e->x + e->width - 1, ACS_VLINE);
    // }

    draw_hline_at(e->y, e->x + 1, e->x + e->width - 1, ACS_HLINE);                 // top border
    draw_vline_at(e->x, e->y + 1, e->y + e->height - 1, ACS_VLINE);                // left border
    draw_vline_at(e->x + e->width - 1, e->y + 1, e->y + e->height - 1, ACS_VLINE); // right border
    draw_hline_at(e->y + 2, e->x + 1, e->x + e->width - 2, ACS_HLINE);             // Entity name seperator
    draw_hline_at(e->y + e->height - 1, e->x + 1, e->x + e->width - 2, ACS_HLINE); // bottom border

    // Corners
    mvaddch(e->y, e->x, ACS_ULCORNER);
    mvaddch(e->y, e->x + e->width - 1, ACS_URCORNER);
    mvaddch(e->y + e->height - 1, e->x, ACS_LLCORNER);
    mvaddch(e->y + e->height - 1, e->x + e->width - 1, ACS_LRCORNER);

    // Entity Name
    attron(A_BOLD);
    mvprintw(e->y + 1, e->x + (e->width - strlen(e->name)) / 2, "%s", e->name);
    attroff(A_BOLD);

    for (int i = 0; i < e->num_properties; i++) {
        if (e->properties[i]) {
            int row = e->y + 3 + i;
            mvprintw(row, e->x + 1, "%-*s", e->width - 2, ""); // clear the row first
            attron(A_BOLD);
            int name_col = e->x + 1;
            if (e->properties[i]->keytype == PRIMARY_KEY) {
                attron(A_UNDERLINE);
                mvprintw(row, name_col, "%s", e->properties[i]->name);
                attroff(A_UNDERLINE);
            } else if (e->properties[i]->keytype == FOREIGN_KEY) {
                mvprintw(row, e->x + 1, "*");
                name_col = e->x + 2;
                mvprintw(row, name_col, "%s", e->properties[i]->name);
            } else {
                mvprintw(row, name_col, "%s", e->properties[i]->name);
            }
            attroff(A_BOLD);
            attron(A_DIM);
            mvprintw(row, name_col + (int)strlen(e->properties[i]->name), ":%s", e->properties[i]->type);
            attroff(A_DIM);
        }
    }
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
    } else if (r->y < 0) {
        r->y = 0;
    } else if (r->y + r->height + CONSOLE_HEIGHT > max_scr_y) {
        int dist = (r->y + r->height + CONSOLE_HEIGHT) - max_scr_y;
        r->y -= dist;
    }

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

    attron(A_BOLD);
    mvprintw(r->y + 1, r->x + (r->width - strlen(r->name)) / 2, "%s", r->name);
    attroff(A_BOLD);
    draw_hline_at(r->y + 2, r->x + 1, r->x + r->width - 2, ACS_HLINE);

    for (int i = 0; i < r->num_properties; i++) {
        if (r->properties[i]) {
            int row = r->y + 3 + i;
            mvprintw(row, r->x + 1, "%-*s", r->width - 2, ""); // clear the row first
            attron(A_BOLD);
            if (r->properties[i]->keytype == PRIMARY_KEY) {
                attron(A_UNDERLINE);
                mvprintw(row, r->x + 1, "%s", r->properties[i]->name);
                attroff(A_UNDERLINE);

            } else { // no foreing key for relationships according to MCD standard
                mvprintw(row, r->x + 1, "%s", r->properties[i]->name);
            }
            attroff(A_BOLD);
            attron(A_DIM);
            mvprintw(row, r->x + 1 + (int)strlen(r->properties[i]->name), ":%s", r->properties[i]->type);
            attroff(A_DIM);
        }
    }

    // added a small diamond outside the corners to satisfy the MCD notation
    // which conventionally draws relationships as diamonds/ovals instead of rectangles
    // for relationships, drawing diamond/oval in ncurses and checking collisions
    // good luck doing that.

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    if (r->y >= 0 && r->x >= 0) {
        mvaddch(r->y, r->x, ACS_DIAMOND);
    }
    if (r->y >= 0 && r->x + r->width - 1 < max_x) {
        mvaddch(r->y, r->x + r->width - 1, ACS_DIAMOND);
    }
    if (r->y + r->height - 1 < max_y && r->x >= 0) {
        mvaddch(r->y + r->height - 1, r->x, ACS_DIAMOND);
    }
    if (r->y + r->height - 1 < max_y && r->x + r->width - 1 < max_x) {
        mvaddch(r->y + r->height - 1, r->x + r->width - 1, ACS_DIAMOND);
    }
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

    int margin = 10; // margin for ?

    // path 1: entity1 --> relationship
    AStarGrid *grid1 = astar_create_grid(start1_x, start1_y, end1_x, end1_y, margin);
    // obstacle marking
    for (int i = 0; i < global_objects.entity_count; i++) {
        if (global_objects.entities[i])
            astar_mark_obstacle(grid1, global_objects.entities[i]->x, global_objects.entities[i]->y,
                                global_objects.entities[i]->width, global_objects.entities[i]->height);
    }
    for (int i = 0; i < global_objects.relationship_count; i++) {
        if (global_objects.relationships[i])
            astar_mark_obstacle(grid1, global_objects.relationships[i]->x, global_objects.relationships[i]->y,
                                global_objects.relationships[i]->width, global_objects.relationships[i]->height);
    }

    AStarPath *path1 = astar_find_path(grid1, start1_x, start1_y, end1_x, end1_y);

    if (path1) {
        add_endpoints_to_path(path1, ap1.x, ap1.y, ap_rel_e1.x, ap_rel_e1.y);
        draw_path_with_corners(path1);
        astar_free_path(path1);
    }
    astar_free_grid(grid1);

    AStarGrid *grid2 = astar_create_grid(start2_x, start2_y, end2_x, end2_y, margin);
    for (int i = 0; i < global_objects.entity_count; i++) {
        if (global_objects.entities[i])
            astar_mark_obstacle(grid2, global_objects.entities[i]->x, global_objects.entities[i]->y,
                                global_objects.entities[i]->width, global_objects.entities[i]->height);
    }
    for (int i = 0; i < global_objects.relationship_count; i++) {
        if (global_objects.relationships[i])
            astar_mark_obstacle(grid2, global_objects.relationships[i]->x, global_objects.relationships[i]->y,
                                global_objects.relationships[i]->width, global_objects.relationships[i]->height);
    }

    AStarPath *path2 = astar_find_path(grid2, start2_x, start2_y, end2_x, end2_y);

    if (path2) {
        // inject the actual border coordinates (ap_rel_e2 and ap2)
        add_endpoints_to_path(path2, ap_rel_e2.x, ap_rel_e2.y, ap2.x, ap2.y);
        draw_path_with_corners(path2);
        astar_free_path(path2);
    }
    astar_free_grid(grid2);

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

void draw_all_entities(GlobalObjects global_objects, int moving_index, bool is_moving) {
    for (int i = 0; i < global_objects.entity_count; i++) {
        if (global_objects.entities[i]) {
            if (is_moving && i == moving_index) {
                attron(COLOR_PAIR(3));
                drawEntity(global_objects.entities[i]);
                attroff(COLOR_PAIR(3));
            } else {
                attron(COLOR_PAIR(7));
                drawEntity(global_objects.entities[i]);
                attroff(COLOR_PAIR(7));
            }
        }
    }
}

void draw_all_relationships(GlobalObjects global_objects, int moving_index, bool is_moving) {
    for (int i = 0; i < global_objects.relationship_count; i++) {
        if (global_objects.relationships[i]) {
            if (is_moving && moving_index == i) {
                attron(COLOR_PAIR(2));
                drawRelationship(global_objects.relationships[i]);
                drawConnection(global_objects.relationships[i]);
                attroff(COLOR_PAIR(2));
            } else {
                attron(COLOR_PAIR(5));
                drawRelationship(global_objects.relationships[i]);
                drawConnection(global_objects.relationships[i]);
                attroff(COLOR_PAIR(5));
            }
        }
    }
}

void draw_all_and_refresh(int screen_width, bool *moving, bool *needs_redraw) {
    // Frame timing
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    moving = false;
    erase();
    mvprintw(0, screen_width / 2 - 10, "MCD Tool - Type 'help' for commands");
    draw_all_entities(global_objects, 0, moving);
    draw_all_relationships(global_objects, 0, moving);
    // queue stdscr and then console_win's doupdate inside draw_console_prompt
    // will flush both together when called right after this in the main loop
    wnoutrefresh(stdscr);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    long us = (t1.tv_sec - t0.tv_sec) * 1000000L + (t1.tv_nsec - t0.tv_nsec) / 1000L;
    mvprintw(0, 0, "frame: %4ldus", us);

    *needs_redraw = false;
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
        }

        local_y++;
        temp = temp->next;
    }

    wnoutrefresh(debug_window);
}
