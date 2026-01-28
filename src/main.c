#include "MCD_elements.h"
#include <ncurses.h>

void drawEntity(Entity e) {
    for (int row = e.y; row < e.y + e.height; row++) {
        for (int col = e.x; col < e.x + e.width; col++) {
            if ((row == e.y && col == e.x) ||
                (row == e.y && col == e.x + e.width - 1) ||
                (row == e.y + e.height - 1 && col == e.x) ||
                (row == e.y + e.height - 1 && col == e.x + e.width - 1)) {
                mvaddch(row, col, '+');
            } else if (row == e.y || row == e.y + e.height - 1 ||
                       row == e.y + 2) {
                mvaddch(row, col, '-');
            } else if (col == e.x || col == e.x + e.width - 1) {
                mvaddch(row, col, '|');
            }
        }
    }
    mvprintw(e.y + 1, e.x + 2, "%s", e.name);
}

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    timeout(16);

    Entity *student = createEntity("Student", 10, 5);
    addProperty(student, "student_id", "int");
    clear();
    printw("Simple MCD Tool - Press 'q' to quit");

    // Draw the entity
    refresh();
    drawEntity(*student);
    refresh();

    int ch;
    while ((ch = getch()) != 'q') {
    }

    endwin();
    return 0;
}
