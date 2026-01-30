#include "graphics.h"
#include "MCD_elements.h"
#include <ncurses.h>

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
                mvaddch(row, col, ACS_LRCORNER);
            } else if (row == e->y + e->height - 1 &&
                       col == e->x + e->width - 1) {
                mvaddch(row, col, ACS_LLCORNER);
            } else if (row == e->y || row == e->y + e->height - 1 ||
                       (row == NAME_BORDER && col != e->x &&
                        col != e->x + e->width - 1)) {
                mvaddch(row, col, ACS_HLINE);
            } else if (col == e->x || col == e->x + e->width - 1) {
                mvaddch(row, col, ACS_VLINE);
            }
        }
    }
    mvprintw(e->y + 1, e->x + 1, "%s", e->name);

#undef NAME_BORDER
#undef CORD_NORM
}

void drawRelationship(Relationship *r) {
#define NAME_BORDER r->y + 2
#define CORD_NORM 1

    for (int i = 0; i < r->num_properties; i++) {
        mvprintw(NAME_BORDER + CORD_NORM + i, r->x + 2, "%s: %s\n",
                 r->properties[i]->name, r->properties[i]->type);
    }
    // hardcoded (improve later)
    for (int i = 0; i < 2; i++) {
        mvprintw(NAME_BORDER + CORD_NORM + i, r->x + 2, "card(%d):%s: \n", i,
                 r->cards[i]->value);
    }

    for (int row = r->y; row < r->y + r->height; row++) {
        for (int col = r->x; col < r->x + r->width; col++) {
            if ((row == r->y && col == r->x) ||
                (row == r->y && col == r->x + r->width - 1) ||
                (row == r->y + r->height - 1 && col == r->x) ||
                (row == r->y + r->height - 1 && col == r->x + r->width - 1)) {
                mvaddch(row, col, '+');
            } else if (row == r->y || row == r->y + r->height - 1 ||
                       row == NAME_BORDER) {
                mvaddch(row, col, '-');
            } else if (col == r->x || col == r->x + r->width - 1) {
                mvaddch(row, col, '|');
            }
        }
    }
    mvprintw(r->y + 1, r->x + 1, "%s", r->name);

#undef NAME_BORDER
#undef CORD_NORM
}
