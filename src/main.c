#include "MCD_elements.h"
#include "global_objects.h"
#include "graphics.h"
#include "parse_commands.h"
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

    Entity *student = createEntity("Student", 10, 2);
    Entity *teacher = createEntity("Teacher", 50, 18);

    addProperty(student, "student_id", "int");
    addProperty(student, "Adress", "str");
    addProperty(student, "Phone_num", "str");

    addProperty(teacher, "Adress", "str");
    addProperty(teacher, "LICENCE", "str");
    addProperty(teacher, "SPECIALITY", "str");

    Relationship *r = addRelationship(30, 20, student, teacher, "Teach");
    addPropertyRelationship(r, "Number_stu", "int");
    addPropertyRelationship(r, "Years_teaching", "int");
    addCardinalityAPI("1,n,n,1", r);

    clear();
    refresh();

    WINDOW *console_win = create_console_window();
    char input_buffer[256] = "";
    int input_len = 0;

    printw("MCD Tool - Type 'help' for commands");

    // drawEntity(student);
    // drawEntity(teacher);
    // drawRelationship(r);
    // drawConnection(r);

    draw_console_prompt(console_win, input_buffer);
    refresh();

    bool is_running = true;
    bool needs_redraw = false;

    while (is_running) {
        if (needs_redraw) {
            move(0, 0);
            clrtobot();

            printw("MCD Tool - Type 'help' for commands");

            for (int i = 0; i < global_objects.entity_count; i++) {
                if (global_objects.entities[i]) {
                    drawEntity(global_objects.entities[i]);
                }
            }

            for (int i = 0; i < global_objects.relationship_count; i++) {
                if (global_objects.relationships[i]) {
                    drawRelationship(global_objects.relationships[i]);
                    drawConnection(global_objects.relationships[i]);
                }
            }

            refresh();
            needs_redraw = false;
        }

        draw_console_prompt(console_win, input_buffer);

        int ch = getch();

        if (ch == ERR) {
            continue;
        }

        if (ch == '\n') {
            if (strcmp(input_buffer, "exit") == 0 ||
                strcmp(input_buffer, "quit") == 0) {
                is_running = false;
            } else {
                execute_command(console_win, input_buffer, &needs_redraw);
            }
            input_len = 0;
            input_buffer[0] = '\0';
        } else if (ch == KEY_BACKSPACE || ch == 127) {
            if (input_len > 0) {
                input_buffer[--input_len] = '\0';
            }
        } else if (ch >= 32 && ch <= 126) {
            if (input_len < 255) {
                input_buffer[input_len++] = ch;
                input_buffer[input_len] = '\0';
            }
        }
    }

    delwin(console_win);
    endwin();
    return 0;
}
