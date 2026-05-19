#include "MCD_elements.h"
#include "command_processor.h"
#include "global_objects.h"
#include "graphics.h"
#include "help_window.h"
#include <ncurses.h>
#include <string.h>

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    initColors();
    enum fps { SIXTY = 60, THIRTY = 30 }; // 16, 33
    enum fps current_fps = THIRTY;

    timeout(current_fps);

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
    char search_buffer[256] = "";
    int search_len = 0;
    HelpAction Action = Navigation;

    // init console_win
    bool moving = false;
    status status = Typing;
    HelpPage page = Main;

    // init help window
    HelpWindow hwin;
    init_help_window(&hwin, Main);

    erase();
    draw_all_entities(global_objects, 0, moving);
    draw_all_relationships(global_objects, 0, moving);
    draw_console_prompt(console_win, input_buffer, status);
    refresh();

    bool is_running = true;
    bool needs_redraw = false;

    while (is_running) {
        if (needs_redraw) {
            draw_all_and_refresh(screen_width, &moving, &needs_redraw);
        }

        switch (status) {
        case Typing:
        case Editing:
            draw_console_prompt(console_win, input_buffer, status); // has it's own refresh
            int ch = getch();
            if (ch == ERR) {
                continue;
            }

            if (ch == '\n') {
                if (strcmp(input_buffer, "exit") == 0 || strcmp(input_buffer, "quit") == 0) {
                    is_running = false;
                } else if (strcmp(input_buffer, "help") == 0) {
                    status = Help;
                } else {
                    da_execute(console_win, input_buffer, &needs_redraw);
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
                // Editing mode
            } else if (ch == '\t') {
                status = Editing;
                int entity_index = 0;
                int relationship_index = 0;
                bool switching_moving_type = true;

                while (switching_moving_type) {
                    int moving_type = getch();
                    bool moving_relationship = false;
                    bool moving_entity = false;
                    moving = true;
                    switch (moving_type) {
                    case 'e':
                        while (moving) {
                            int move_key = getch();
                            if (entity_index >= global_objects.entity_count) {
                                entity_index = global_objects.entity_count % entity_index;
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
                            case 'q':
                                moving = false;
                                break;
                            }
                            erase();
                            draw_all_relationships(global_objects, relationship_index, moving_relationship);
                            draw_all_entities(global_objects, entity_index, moving);
                            refresh();
                            // TODO : implement a public API to choose whether to make call a draw
                            //  help console or a command console func
                            draw_console_prompt(console_win, input_buffer, status);
                        }
                        break;
                    case 'r':
                        while (moving) {
                            int move_key = getch();
                            if (relationship_index >= global_objects.relationship_count) {
                                relationship_index = global_objects.relationship_count % relationship_index;
                            }
                            switch (move_key) {
                            case KEY_UP:
                                global_objects.relationships[relationship_index]->y -= 1;
                                break;
                            case KEY_DOWN:
                                global_objects.relationships[relationship_index]->y += 1;
                                break;
                            case KEY_RIGHT:
                                global_objects.relationships[relationship_index]->x += 1;
                                break;
                            case KEY_LEFT:
                                global_objects.relationships[relationship_index]->x -= 1;
                                break;
                            case '\t':
                                relationship_index++;
                                break;
                            case 'q':
                                moving = false;
                                break;
                            }
                            erase();
                            draw_all_entities(global_objects, entity_index, moving_entity);
                            draw_all_relationships(global_objects, relationship_index, moving);
                            refresh();
                            draw_console_prompt(console_win, input_buffer, status);
                        }
                        break;

                    case 'x':
                        switching_moving_type = false;
                        status = Typing;
                        break;
                    }
                }
            }
            break;
        case Help:
            set_current_page(&hwin, page);
            draw_help_window(console_win, &hwin, search_buffer, page, Action);
            int wch = getch();
            if (wch == ERR) {
                continue;
            }

            if (wch == 'q') {
                // revert changes
                status = Typing;
                int screen_height, screen_width;
                getmaxyx(stdscr, screen_height, screen_width);

                int console_y = screen_height - CONSOLE_HEIGHT;

                wresize(console_win, CONSOLE_HEIGHT, screen_width);
                mvwin(console_win, console_y, 0);
                werase(console_win);

                needs_redraw = true;
            } else if (wch == 'm') {
                // TODO: why escape takes long here and in editing mode (replaced with q)
                // This is for commands
                // implement a search for command and highlight
                page = Main;
            } else if (wch == 'h') {
                page = Hotkeys;
            } else if (wch == 'e') {
                page = Examples;
            } else if (page == Main && wch == '/') {
                Action = Search;
                while (Action) {
                    draw_help_window(console_win, &hwin, search_buffer, page, Action);
                    int search_char = getch();
                    if (search_char == ERR) {
                        continue;
                    }
                    if (search_char == '\n') {
                        // do_search(search_buffer);
                        search_len = 0;
                        search_buffer[0] = '\0';
                    } else if (search_char == 'q') {
                        Action = Navigation;
                        search_len = 0;
                        search_buffer[0] = '\0';
                    } else if (search_char == KEY_BACKSPACE || search_char == 127) {
                        if (search_len > 0) {
                            search_buffer[--search_len] = '\0';
                        }
                    } else if (search_char >= 32 && search_char <= 126) {
                        if (search_len < 255) {
                            search_buffer[search_len++] = search_char;
                            search_buffer[search_len] = '\0';
                        }
                    }
                }
            }
            break;
        }
    }
    delwin(console_win);
    endwin();
    return 0;
}
