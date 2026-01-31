#include "graphics.h"
#include "MCD_elements.h"
#include <ncurses.h>
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
