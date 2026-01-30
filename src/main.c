#include "MCD_elements.h"
#include "graphics.h"
#include <ncurses.h>

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    timeout(16);

    Entity *student = createEntity("Student", 10, 5);
    Entity *teacher = createEntity("Teacher", 10, 15);
    addProperty(student, "student_id", "int");
    addProperty(student, "Adress", "string");
    addProperty(student, "Phone_num", "str");
    addProperty(student, "Wilaya", "str");
    Relationship *r = addRelationship(40, 5, student, teacher, "Teach");
    addCardinalityAPI("1,n,n,1", r);

    clear();
    printw("Simple MCD Tool - Press 'q' to quit");

    // Draw the entity
    drawEntity(student);
    drawEntity(teacher);
    drawRelationship(r);

    refresh();

    int ch;
    while ((ch = getch()) != 'q') {
    }

    endwin();
    return 0;
}
