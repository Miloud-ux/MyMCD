#include "graphics.h"
#include "DSA/astar.h"
#include <ncurses.h>
#include <string.h>

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
    // ncurses hline(ch, n) draws n characters.
    // To go from 10 to 12, we need 3 characters (10, 11, 12).
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
    // TODO : Reimplement this using vline and hline

    mvaddch(e->y, e->x, ACS_ULCORNER);
    for (int i = 1; i < e->width - 1; i++) {
        mvaddch(e->y, e->x + i, ACS_HLINE);
    }
    mvaddch(e->y, e->x + e->width - 1, ACS_URCORNER);

    for (int i = 1; i < e->height - 1; i++) {
        mvaddch(e->y + i, e->x, ACS_VLINE);
        mvprintw(e->y + i, e->x + 1, "%-*s", e->width - 2, "");
        // why is this -2 ?  at least -1 ? does this affect A* ?
        mvaddch(e->y + i, e->x + e->width - 1, ACS_VLINE);
    }

    mvaddch(e->y + e->height - 1, e->x, ACS_LLCORNER);
    for (int i = 1; i < e->width - 1; i++) {
        mvaddch(e->y + e->height - 1, e->x + i, ACS_HLINE);
    }
    mvaddch(e->y + e->height - 1, e->x + e->width - 1, ACS_LRCORNER);
    mvprintw(e->y + 1, e->x + (e->width - strlen(e->name)) / 2, "%s", e->name);
    draw_hline_at(e->y + 2, e->x + 1, e->x + e->width - 2, ACS_HLINE);

    for (int i = 0; i < e->num_properties; i++) {
        if (e->properties[i]) {
            char prop_str[64];
            snprintf(prop_str, sizeof(prop_str), "%s:%s", e->properties[i]->name, e->properties[i]->type);
            mvprintw(e->y + 3 + i, e->x + 1, "%-*s", e->width - 2, prop_str);
        }
    }
}

void drawRelationship(Relationship *r) {

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
    draw_hline_at(r->y + 2, r->x + 1, r->x + r->width - 2, ACS_HLINE);

    for (int i = 0; i < r->num_properties; i++) {
        if (r->properties[i]) {
            char prop_str[64];
            snprintf(prop_str, sizeof(prop_str), "%s:%s", r->properties[i]->name, r->properties[i]->type);
            mvprintw(r->y + 3 + i, r->x + 1, "%-*s", r->width - 2, prop_str);
        }
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
        if (has_down && has_right) {
            mvaddch(curr_y, curr_x, ACS_ULCORNER); // ┌
        } else if (has_down && has_left) {
            mvaddch(curr_y, curr_x, ACS_URCORNER); // ┐
        } else if (has_up && has_right) {
            mvaddch(curr_y, curr_x, ACS_LLCORNER); // └
        } else if (has_up && has_left) {
            mvaddch(curr_y, curr_x, ACS_LRCORNER); // ┘
        }
    }

    attroff(COLOR_PAIR(3));
}

void drawConnectionAStar(Relationship *r) {
    if (!r || !r->e1 || !r->e2)
        return;

    AttachPoint ap1 = findBestAttachPoint(r->e1->x, r->e1->y, r->e1->width, r->e1->height, r->x, r->y);
    AttachPoint ap_rel_e1 = findBestAttachPoint(r->x, r->y, r->width, r->height, r->e1->x, r->e1->y);
    AttachPoint ap_rel_e2 = findBestAttachPoint(r->x, r->y, r->width, r->height, r->e2->x, r->e2->y);
    AttachPoint ap2 = findBestAttachPoint(r->e2->x, r->e2->y, r->e2->width, r->e2->height, r->x, r->y);

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
    if (r->cards[0])
        mvprintw(ap1.y, ap1.x, "%s", r->cards[0]->value);
    if (r->cards[1])
        mvprintw(ap2.y, ap2.x, "%s", r->cards[1]->value);
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

        wrefresh(console_win);

    } else if (status == Help) {
        int h = 22;
        int w = 80;

        if (h >= std_screen_height - 2)
            h = std_screen_height - 2;
        if (w >= std_screen_width - 2)
            w = std_screen_width - 2;

        int center_y = (std_screen_height - h) / 2;
        int center_x = (std_screen_width - w) / 2;

        wresize(console_win, h, w);
        mvwin(console_win, center_y, center_x);

        werase(console_win);
        box(console_win, 0, 0);

        // header
        wattron(console_win, COLOR_PAIR(6) | A_BOLD | A_REVERSE);
        mvwprintw(console_win, 1, (w / 2) - 14, "  MCD TOOL COMMAND GUIDE  ");
        wattroff(console_win, COLOR_PAIR(6) | A_BOLD | A_REVERSE);

        // section: Syntax
        wattron(console_win, COLOR_PAIR(6) | A_BOLD | A_UNDERLINE);
        mvwprintw(console_win, 4, 4, "COMMANDS");
        wattroff(console_win, COLOR_PAIR(6) | A_BOLD | A_UNDERLINE);

        // item 1
        mvwprintw(console_win, 6, 6, ">> ");
        wattron(console_win, A_BOLD);
        waddstr(console_win, "create entity ");
        wattroff(console_win, A_BOLD);
        wattron(console_win, A_DIM);
        waddstr(console_win, "<name>");
        wattroff(console_win, A_DIM);

        // item 2
        mvwprintw(console_win, 7, 6, ">> ");
        wattron(console_win, A_BOLD);
        waddstr(console_win, "create relationship ");
        wattroff(console_win, A_BOLD);
        wattron(console_win, A_DIM);
        waddstr(console_win, "<name> <e1> <e2>");
        wattroff(console_win, A_DIM);

        // item 3
        mvwprintw(console_win, 8, 6, ">> ");
        wattron(console_win, A_BOLD);
        waddstr(console_win, "add property ");
        wattroff(console_win, A_BOLD);
        wattron(console_win, A_DIM);
        waddstr(console_win, "\"target\" \"prop\" type");
        wattroff(console_win, A_DIM);

        // section: controls
        // wattron(console_win, COLOR_PAIR(4) | A_BOLD | A_UNDERLINE);
        // mvwprintw(console_win, 11, 4, "HOTKEYS");
        // wattroff(console_win, COLOR_PAIR(4) | A_BOLD | A_UNDERLINE);
        //
        // mvwprintw(console_win, 13, 6, "[TAB] Enter Edit Mode     [q]   Close Help");
        // mvwprintw(console_win, 14, 6, "[ESC] Back to Typing      [x]   Exit Movement");
        // mvwprintw(console_win, 15, 6, "[ARROWS] Move Elements");

        wattron(console_win, COLOR_PAIR(7) | A_BOLD | A_REVERSE);
        mvwprintw(console_win, h - 2, (w / 2) - 11, "Press 'q' to exit");
        wattroff(console_win, COLOR_PAIR(7) | A_BOLD | A_REVERSE);

        wattron(console_win, COLOR_PAIR(7) | A_BOLD | A_REVERSE);
        mvwprintw(console_win, h - 2, 2, "Press 'h' for hotkeys");
        mvwprintw(console_win, h - 2, w - 24, "Press 'e' for examples");
        wattroff(console_win, COLOR_PAIR(7) | A_BOLD | A_REVERSE);
        wrefresh(console_win);
    }
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
