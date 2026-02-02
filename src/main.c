#include "MCD_elements.h"
#include "global_objects.h"
#include "graphics.h"
#include <ncurses.h>

int main() {
    // ... initialization code ...
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    initColors();
    timeout(16);

    init_global_objects();

    Entity *student = createEntity("Student", 10, 1);
    Entity *teacher = createEntity("Teacher", 90, 10);

    addProperty(student, "student_id", "int");
    addProperty(student, "Adress", "str");
    addProperty(student, "Phone_num", "str");

    addProperty(teacher, "Adress", "str");
    addProperty(teacher, "LICENCE", "str");
    addProperty(teacher, "SPECIALITY", "str");

    Relationship *r = addRelationship(40, 20, student, teacher, "Teach");
    addPropertyRelationship(r, "Number_stu", "int");
    addPropertyRelationship(r, "Years_teaching", "int");
    addCardinalityAPI("1,n,n,1", r);

    clear();
    printw("MCD Tool with A* Pathfinding - Press 'q' to quit");

    // Add debug info
    mvprintw(1, 0, "Student: (%d,%d) %dx%d", student->x, student->y,
             student->width, student->height);
    mvprintw(2, 0, "Teacher: (%d,%d) %dx%d", teacher->x, teacher->y,
             teacher->width, teacher->height);
    mvprintw(3, 0, "Relation: (%d,%d) %dx%d", r->x, r->y, r->width, r->height);

    drawEntity(student);
    drawEntity(teacher);
    drawRelationship(r);
    drawConnection(r);

    refresh();

    int ch;
    while ((ch = getch()) != 'q') {
    }

    endwin();
    return 0;
}
