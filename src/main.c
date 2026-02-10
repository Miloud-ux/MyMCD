#include "MCD_elements.h"
#include "global_objects.h"
#include "graphics.h"
#include <ncurses.h>
#include <string.h>

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    initColors();
    timeout(16);

    init_global_objects();

    Entity *student = createEntity("Student", 60, 2);
    Entity *teacher = createEntity("Teacher", 100, 18);

    addProperty(student, "student_id", "int");
    addProperty(student, "Adress", "str");
    addProperty(student, "Phone_num", "str");

    addProperty(teacher, "Adress", "str");
    addProperty(teacher, "LICENCE", "str");
    addProperty(teacher, "SPECIALITY", "str");

    Relationship *r = addRelationship(1, 2, student, teacher, "Teach");
    addPropertyRelationship(r, "Number_stu", "int");
    addPropertyRelationship(r, "Years_teaching", "int");
    addCardinalityAPI("1,n,n,1", r);

    // Clear screen once at the beginning
    clear();
    refresh();

    // Create console window
    WINDOW *console_win = create_console_window();
    char input_buffer[256] = "";
    int input_len = 0;

    // Draw diagram area
    printw("MCD Tool with A* Pathfinding");
    mvprintw(1, 0, "Student: (%d,%d) %dx%d", student->x, student->y,
             student->width, student->height);
    mvprintw(2, 0, "Teacher: (%d,%d) %dx%d", teacher->x, teacher->y,
             teacher->width, teacher->height);
    mvprintw(3, 0, "Relation: (%d,%d) %dx%d", r->x, r->y, r->width, r->height);

    drawEntity(student);
    drawEntity(teacher);
    drawRelationship(r);
    drawConnection(r);

    // Draw console for the first time
    draw_console_prompt(console_win, input_buffer);

    // Refresh main screen
    refresh();

    bool is_running = true;
    bool needs_redraw = false;

    while (is_running) {
        // Redraw diagram if needed
        if (needs_redraw) {
            move(0, 0);
            clrtobot(); // Clear diagram area only

            printw("MCD Tool with A* Pathfinding");
            mvprintw(1, 0, "Student: (%d,%d) %dx%d", student->x, student->y,
                     student->width, student->height);
            mvprintw(2, 0, "Teacher: (%d,%d) %dx%d", teacher->x, teacher->y,
                     teacher->width, teacher->height);
            mvprintw(3, 0, "Relation: (%d,%d) %dx%d", r->x, r->y, r->width,
                     r->height);

            drawEntity(student);
            drawEntity(teacher);
            drawRelationship(r);
            drawConnection(r);

            refresh();
            needs_redraw = false;
        }

        // Always update console
        draw_console_prompt(console_win, input_buffer);

        int ch = getch();

        if (ch == ERR) {
            continue;
        }

        // Handle quit
        if (ch == 'q' && input_len == 0) {
            mvwprintw(console_win, 1, 1, "Bye! Press any key to exit...");
            wrefresh(console_win);
            getch();
            is_running = false;
        }
        // Handle Enter (process command)
        else if (ch == '\n') {
            // Process command here (you'll implement this)
            mvprintw(5, 0, "Command: %s", input_buffer);

            // Example command: "move student right"
            if (strcmp(input_buffer, "move student right") == 0) {
                student->x += 10;
                needs_redraw = true;
            } else if (strcmp(input_buffer, "move student left") == 0) {
                student->x -= 10;
                needs_redraw = true;
            }

            // Clear input
            input_len = 0;
            input_buffer[0] = '\0';
        }
        // Handle backspace
        else if (ch == KEY_BACKSPACE || ch == 127) {
            if (input_len > 0) {
                input_buffer[--input_len] = '\0';
            }
        }
        // Handle printable characters
        else if (ch >= 32 && ch <= 126) {
            if (input_len < 255) {
                input_buffer[input_len++] = ch;
                input_buffer[input_len] = '\0';
            }
        }
        // Test movement with arrow keys
        else if (ch == KEY_RIGHT) {
            student->x += 5;
            needs_redraw = true;
        } else if (ch == KEY_LEFT) {
            student->x -= 5;
            needs_redraw = true;
        }
    }

    delwin(console_win);
    endwin();
    return 0;
}
