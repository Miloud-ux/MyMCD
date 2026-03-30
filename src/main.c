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

    int screen_height, screen_width;
    getmaxyx(stdscr, screen_height, screen_width);

    init_global_objects();

    Entity *student = createEntity("Student", 3, 20);
    Entity *teacher = createEntity("Teacher", 80, 18);

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

    WINDOW *console_win = create_console_window();
    char input_buffer[256] = "";
    int input_len = 0;
    bool moving = false;

    draw_all_entities(global_objects, 0, moving);
    draw_all_relationships(global_objects, 0, moving);
    draw_console_prompt(console_win, input_buffer);
    refresh();

    bool is_running = true;
    bool needs_redraw = false;

    while (is_running) {
        if (needs_redraw) {
            erase();
            mvprintw(0, screen_width / 2 - 10,
                     "MCD Tool - Type 'help' for commands");
            draw_all_entities(global_objects, 0, moving);
            draw_all_relationships(global_objects, 0, moving);

            refresh();

            // TODO : see which one is better for performance
            // wnoutrefresh(stdscr);
            // wnoutrefresh(console_win);
            // doupdate();

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
        } else if (ch == '\t') {
            int entity_index = 0;
            int relationship_index = 0;
            moving = true;
            bool switching_moving_type = true;

            while (switching_moving_type) {
                int moving_type = getch();
                switch (moving_type) {
                case 'e':
                    while (moving) {
                        int move_key = getch();
                        if (entity_index >= global_objects.entity_count) {
                            entity_index =
                                global_objects.entity_count % entity_index;
                        }
                        if (relationship_index >=
                            global_objects.relationship_count) {
                            relationship_index =
                                global_objects.relationship_count %
                                relationship_index;
                        }

                        switch (move_key) {
                        case KEY_UP:
                            global_objects.entities[entity_index]->y -= 1;
                            break;
                        case KEY_DOWN:
                            global_objects.entities[entity_index]->y += 1;
                            break;
                        case KEY_RIGHT:
                            global_objects.entities[entity_index]->x += 1;
                            break;
                        case KEY_LEFT:
                            global_objects.entities[entity_index]->x -= 1;
                            break;
                        case '\t':
                            entity_index++;
                            break;
                        case 27:
                            moving = false;
                            break;
                        }
                        erase();
                        draw_all_relationships(global_objects,
                                               relationship_index, moving);
                        draw_all_entities(global_objects, entity_index, moving);

                        refresh();

                        // TODO : see which one is better for peformance
                        // wnoutrefresh(stdscr);
                        // wnoutrefresh(console_win);
                        // doupdate();
                    }
                    break;

                case 'r':
                    while (moving) {
                        int move_key = getch();
                        if (relationship_index >=
                            global_objects.relationship_count) {
                            relationship_index =
                                global_objects.relationship_count %
                                relationship_index;
                        }

                        switch (move_key) {
                        case KEY_UP:
                            global_objects.relationships[relationship_index]
                                ->y -= 1;
                            break;
                        case KEY_DOWN:
                            global_objects.relationships[relationship_index]
                                ->y += 1;
                            break;
                        case KEY_RIGHT:
                            global_objects.relationships[relationship_index]
                                ->x += 1;
                            break;
                        case KEY_LEFT:
                            global_objects.relationships[relationship_index]
                                ->x -= 1;
                            break;
                        case '\t':
                            relationship_index++;
                            break;
                        case 27:
                            moving = false;
                            break;
                        }
                        erase();
                        draw_all_relationships(global_objects,
                                               relationship_index, moving);
                        refresh();
                    }
                    break;

                case 'x':
                    switching_moving_type = false;
                    break;
                }
            }

            needs_redraw = true;
        }
    }
    delwin(console_win);
    endwin();
    return 0;
}
