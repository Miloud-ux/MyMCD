#include "graphics.h"
#include "MCD_elements.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

void drawEntity(Entity *e) {
#define NAME_BORDER e->y + 2
#define CORD_NORM 1

    for (int i = 0; i < e->num_properties; i++) {
        mvprintw(NAME_BORDER + CORD_NORM + i, e->x + 2, "%s: %s\n",
                 e->properties[i]->name, e->properties[i]->type);
    }

    for (int row = e->y; row < e->y + e->height; row++) {
        for (int col = e->x; col < e->x + e->width; col++) {
            if ((row == e->y && col == e->x)) {
                mvaddch(row, col, ACS_ULCORNER);
            } else if (row == e->y && col == e->x + e->width - 1) {
                mvaddch(row, col, ACS_URCORNER);
            } else if (row == e->y + e->height - 1 && col == e->x) {
                mvaddch(row, col, ACS_LLCORNER);
            } else if (row == e->y + e->height - 1 &&
                       col == e->x + e->width - 1) {
                mvaddch(row, col, ACS_LRCORNER);
            } else if (row == e->y || row == e->y + e->height - 1 ||
                       (row == NAME_BORDER && col != e->x &&
                        col != e->x + e->width - 1)) {
                mvaddch(row, col, ACS_HLINE);
            } else if (col == e->x || col == e->x + e->width - 1) {
                mvaddch(row, col, ACS_VLINE);
            }
        }
    }

    int center_x = e->x + e->width / 2;

    int name_start_col = center_x - strlen(e->name) / 2;

    if (name_start_col < e->x + 1) {
        name_start_col = e->x + 1;
    }

    mvprintw(e->y + 1, name_start_col, "%s", e->name);

#undef NAME_BORDER
#undef CORD_NORM
}

void initColors() {
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);   // Normal relationship
    init_pair(2, COLOR_YELLOW, COLOR_BLACK); // Selected relationship
    init_pair(3, COLOR_GREEN, COLOR_BLACK);  // Entities
    init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
}

void drawRelationship(Relationship *r) {
    if (!r)
        return;
    attron(COLOR_PAIR(1)); // Turn on color pair 1
    int center_x = r->x + r->width / 2;

    for (int i = 0; i < r->num_properties; i++) {
        if (r->properties[i]) {
            mvprintw(r->y + 3 + i, r->x + 2, "%s: %s", r->properties[i]->name,
                     r->properties[i]->type);
        }
    }

    for (int row = r->y; row < r->y + r->height; row++) {
        for (int col = r->x; col < r->x + r->width; col++) {
            if ((row == r->y && col == r->x) ||
                (row == r->y && col == r->x + r->width - 1) ||
                (row == r->y + r->height - 1 && col == r->x) ||
                (row == r->y + r->height - 1 && col == r->x + r->width - 1)) {
                mvaddch(row, col, ACS_DIAMOND);
            }

            else if (row == r->y || row == r->y + r->height - 1 ||
                     row == r->y + 2) {
                mvaddch(row, col, ACS_HLINE);
            }

            else if (col == r->x || col == r->x + r->width - 1) {
                mvaddch(row, col, ACS_VLINE);
            }
        }
    }

    int name_start_col = center_x - strlen(r->name) / 2;

    if (name_start_col < r->x + 1) {
        name_start_col = r->x + 1;
    }

    mvprintw(r->y + 1, name_start_col, "%s", r->name);

    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(4));

    if (r->cards[0] && r->cards[1]) {
        mvprintw(r->y - 1, r->x, "%s", r->cards[0]->value);
        int right_card_x = r->x + r->width - strlen(r->cards[1]->value);
        mvprintw(r->y - 1, right_card_x, "%s", r->cards[1]->value);
    }
    attroff(COLOR_PAIR(4));
}

// Helper functions for drawing lines between specific coordinates
void draw_hline_at(int y, int x1, int x2, chtype ch) {
    int start = (x1 < x2) ? x1 : x2;
    int end = (x1 < x2) ? x2 : x1;

    for (int x = start; x <= end; x++) {
        mvaddch(y, x, ch);
    }
}

void draw_vline_at(int x, int y1, int y2, chtype ch) {
    int start = (y1 < y2) ? y1 : y2;
    int end = (y1 < y2) ? y2 : y1;

    for (int y = start; y <= end; y++) {
        mvaddch(y, x, ch);
    }
}

// Main connection drawing function - AVOIDS BOXES
void drawConnection(Relationship *r) {
    if (!r || !r->e1 || !r->e2)
        return;

    Entity *e1 = r->e1;
    Entity *e2 = r->e2;

    // Calculate centers for reference
    int e1_cx = e1->x + e1->width / 2;
    int e1_cy = e1->y + e1->height / 2;
    int e2_cx = e2->x + e2->width / 2;
    int e2_cy = e2->y + e2->height / 2;
    int r_cx = r->x + r->width / 2;
    int r_cy = r->y + r->height / 2;

    // =========================================================
    // Connection 1: Entity1 -> Relationship
    // =========================================================

    // Find where to exit entity1 and enter relationship
    AttachPoint e1_point =
        findBestAttachPoint(e1->x, e1->y, e1->width, e1->height, r_cx, r_cy);
    AttachPoint r1_point =
        findBestAttachPoint(r->x, r->y, r->width, r->height, e1_cx, e1_cy);

    // Draw the line - key insight: go OUTSIDE boxes first
    if (e1_point.side == SIDE_LEFT || e1_point.side == SIDE_RIGHT) {
        // Exit from left/right: go horizontal first, then vertical
        // Start 1 unit away from the entity to avoid box
        int start_x =
            (e1_point.side == SIDE_LEFT) ? e1_point.x - 1 : e1_point.x + 1;

        // Draw horizontal to above/below relationship
        draw_hline_at(e1_point.y, start_x, r1_point.x, ACS_HLINE);

        // Draw vertical to relationship entry point
        if (e1_point.y < r1_point.y) {
            draw_vline_at(r1_point.x, e1_point.y, r1_point.y, ACS_VLINE);
            mvaddch(e1_point.y, r1_point.x, ACS_ULCORNER);
        } else {
            draw_vline_at(r1_point.x, r1_point.y, e1_point.y, ACS_VLINE);
            mvaddch(e1_point.y, r1_point.x, ACS_LLCORNER);
        }
    } else {
        // Exit from top/bottom: go vertical first, then horizontal
        int start_y =
            (e1_point.side == SIDE_TOP) ? e1_point.y - 1 : e1_point.y + 1;

        // Draw vertical away from entity
        draw_vline_at(e1_point.x, start_y, r1_point.y, ACS_VLINE);

        // Draw horizontal to relationship
        if (e1_point.x < r1_point.x) {
            draw_hline_at(r1_point.y, e1_point.x, r1_point.x, ACS_HLINE);
            mvaddch(r1_point.y, e1_point.x, ACS_ULCORNER);
        } else {
            draw_hline_at(r1_point.y, r1_point.x, e1_point.x, ACS_HLINE);
            mvaddch(r1_point.y, e1_point.x, ACS_URCORNER);
        }
    }

    // =========================================================
    // Connection 2: Relationship -> Entity2
    // =========================================================

    AttachPoint e2_point =
        findBestAttachPoint(e2->x, e2->y, e2->width, e2->height, r_cx, r_cy);
    AttachPoint r2_point =
        findBestAttachPoint(r->x, r->y, r->width, r->height, e2_cx, e2_cy);

    // Draw the second line
    if (r2_point.side == SIDE_LEFT || r2_point.side == SIDE_RIGHT) {
        // Exit from relationship left/right
        int start_x =
            (r2_point.side == SIDE_LEFT) ? r2_point.x - 1 : r2_point.x + 1;

        draw_hline_at(r2_point.y, start_x, e2_point.x, ACS_HLINE);

        if (r2_point.y < e2_point.y) {
            draw_vline_at(e2_point.x, r2_point.y, e2_point.y, ACS_VLINE);
            mvaddch(r2_point.y, e2_point.x, ACS_ULCORNER);
        } else {
            draw_vline_at(e2_point.x, e2_point.y, r2_point.y, ACS_VLINE);
            mvaddch(r2_point.y, e2_point.x, ACS_LLCORNER);
        }
    } else {
        // Exit from relationship top/bottom
        int start_y =
            (r2_point.side == SIDE_TOP) ? r2_point.y - 1 : r2_point.y + 1;

        draw_vline_at(r2_point.x, start_y, e2_point.y, ACS_VLINE);

        if (r2_point.x < e2_point.x) {
            draw_hline_at(e2_point.y, r2_point.x, e2_point.x, ACS_HLINE);
            mvaddch(e2_point.y, r2_point.x, ACS_ULCORNER);
        } else {
            draw_hline_at(e2_point.y, e2_point.x, r2_point.x, ACS_HLINE);
            mvaddch(e2_point.y, r2_point.x, ACS_URCORNER);
        }
    }

    // Draw cardinalities if they exist
    if (r->cards[0]) {
        // Position cardinality near entity1 connection point
        int card_x = e1_point.x;
        int card_y = e1_point.y;

        // Offset based on which side we're on
        if (e1_point.side == SIDE_TOP) {
            card_y -= 2;
        } else if (e1_point.side == SIDE_BOTTOM) {
            card_y += 1;
        } else if (e1_point.side == SIDE_LEFT) {
            card_x -= (strlen(r->cards[0]->value) + 1);
        } else {
            card_x += 1;
        }

        mvprintw(card_y, card_x, "%s", r->cards[0]->value);
    }

    if (r->cards[1]) {
        int card_x = e2_point.x;
        int card_y = e2_point.y;

        if (e2_point.side == SIDE_TOP) {
            card_y -= 2;
        } else if (e2_point.side == SIDE_BOTTOM) {
            card_y += 1;
        } else if (e2_point.side == SIDE_LEFT) {
            card_x -= (strlen(r->cards[1]->value) + 1);
        } else {
            card_x += 1;
        }

        mvprintw(card_y, card_x, "%s", r->cards[1]->value);
    }
}
