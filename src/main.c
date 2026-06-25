#include "MCD_elements.h"
#include "command_processor.h"
#include "global_objects.h"
#include "graphics.h"
#include "help_window.h"
#include "utils/arena_allocator.h"
#include "utils/menu.h"
#include "utils/save.h"
#include <ncurses.h>
#include <string.h>

#define MAX_COMMAND_HISTORY 50

// Test diagram
static void setup_large_e_commerce_delivery_mcd(void);

int main() {
    initscr();

    initColors();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    set_escdelay(50);

    // TODO: move to graphics.h header
    timeout(16); // 60fps and 30 for 30fps

    int screen_height, screen_width;
    getmaxyx(stdscr, screen_height, screen_width);

    init_global_objects();
    // test diagram

    // == WINDOWS ==
    WINDOW *debug_window = create_ast_debug_window();
    WINDOW *console_win = create_console_window();
    WINDOW *help_win = create_help_window();
    char input_buffer[256] = "";
    int input_len = 0;
    char command_history[MAX_COMMAND_HISTORY][256];
    int history_count = 0;
    int history_index = -1;
    char search_buffer[MAX_SEARCH_BUFFER_LEN] = "";
    int search_len = 0;
    HelpAction Action = Navigation;

    // Stdscreen info
    int std_screen_width, std_screen_height;
    getmaxyx(stdscr, std_screen_height, std_screen_width);

    // Help window info
    // int center_y = (std_screen_height - HELP_WIN_HEIGHT) / 2;
    // int center_x = (std_screen_width - HELP_WIN_WIDTH) / 2;
    int curr_main_scrolling_line;
    int curr_examples_scrolling_line;
    int curr_hotkey_scrolling_line;

    // init console_win
    bool moving = false;
    status status = Typing;
    last_status = Typing;

    HelpPage page = Main;

    // init help window
    HelpWindow hwin;
    init_help_window(&hwin, page);

    // init scrolling pad
    WINDOW *scrolling_pad = init_pad(PAD_LINES, PAD_COLS, hwin);

    bool is_running = true;
    bool needs_redraw = false;

    // === Startup menu ========================================================
    // show_startup_menu() blocks until the user picks an option.  It uses its
    // own temporary window and cleans up before returning, so the persistent
    // windows below are unaffected.
    MenuChoice startup_choice = show_startup_menu();
    if (startup_choice == MENU_EXIT) {
        delwin(console_win);
        delwin(help_win);
        delwin(debug_window);
        delwin(scrolling_pad);
        endwin();
        return 0;
    }

    // Arena and AST must exist before load_diagram() is called.
    Arena a;
    arena_init(&a, 1024 * 1024);
    AST tree;
    init_AST(&tree);

    if (startup_choice == MENU_NEW) {
        // Load the built-in test diagram (original behaviour)
        setup_large_e_commerce_delivery_mcd();
    } else {
        // MENU_LOAD: prompt for filename, then replay commands from the file.
        // Draw a simple centred prompt directly on stdscr.
        char load_filename[256] = "";
        int load_len = 0;

        int sh, sw;
        getmaxyx(stdscr, sh, sw);
        int box_h = 5, box_w = 54;
        int box_y = (sh - box_h) / 2;
        int box_x = (sw - box_w) / 2;
        if (box_y < 0)
            box_y = 0;
        if (box_x < 0)
            box_x = 0;

        // Draw a simple box + prompt
        WINDOW *load_win = newwin(box_h, box_w, box_y, box_x);
        keypad(load_win, TRUE);
        box(load_win, 0, 0);
        wattron(load_win, A_BOLD | COLOR_PAIR(2));
        mvwprintw(load_win, 1, 2, "Load Diagram");
        wattroff(load_win, A_BOLD | COLOR_PAIR(2));
        mvwprintw(load_win, 2, 2, "Filename: ");
        wrefresh(load_win);
        echo();
        curs_set(1);
        // Read filename using wgetnstr for simplicity; it echoes in-window
        mvwgetnstr(load_win, 2, 12, load_filename, 253);
        noecho();
        curs_set(0);
        delwin(load_win);
        clear();
        refresh();

        load_len = (int)strlen(load_filename);
        // Trim trailing whitespace / newlines just in case
        while (load_len > 0 && (load_filename[load_len - 1] == '\n' || load_filename[load_len - 1] == '\r' ||
                                load_filename[load_len - 1] == ' ')) {
            load_filename[--load_len] = '\0';
        }

        if (load_len > 0) {
            load_diagram(load_filename, &a, &tree, console_win, &needs_redraw);

            // debug
            // for (int i = 0; i < global_objects.entity_count; i++) {
            //     if (global_objects.entities[i])
            //         mvprintw(i + 1, 0, "ENTITY %d: %s", i, global_objects.entities[i]->name);
            // }
            // refresh();
            // getch();
        }
    }

    // Drawing
    draw_all_and_refresh(screen_width, &moving, &needs_redraw);
    draw_console_prompt(console_win, input_buffer, status);

    while (is_running) {

        if (needs_redraw) {
            draw_all_and_refresh(screen_width, &moving, &needs_redraw);
        }

        switch (status) {
        case Typing:
        case Editing:
            draw_console_prompt(console_win, input_buffer, status);
            int ch = getch();
            if (ch == ERR) {
                continue;
            }

            if (ch == '\n') {
                if (strlen(input_buffer) > 0) {
                    if (history_count < MAX_COMMAND_HISTORY) {
                        strncpy(command_history[history_count], input_buffer, 255);
                        command_history[history_count][255] = '\0';
                        history_count++;
                    } else {
                        for (int hist_i = 1; hist_i < MAX_COMMAND_HISTORY; hist_i++) {
                            strcpy(command_history[hist_i - 1], command_history[hist_i]);
                        }
                        strncpy(command_history[MAX_COMMAND_HISTORY - 1], input_buffer, 255);
                        command_history[MAX_COMMAND_HISTORY - 1][255] = '\0';
                    }
                }
                if (strcmp(input_buffer, "exit") == 0 || strcmp(input_buffer, "quit") == 0) {
                    is_running = false;
                } else if (strcmp(input_buffer, "help") == 0) {
                    status = Help;
                    last_status = Help;
                    curs_set(0); // UPDATE => hide cursor while help is open, cursor warping is the flicker
                } else if (strlen(input_buffer) == 0) {
                    // mvwprintw(stdscr, screen_height / 2, screen_width / 2, "UPDATED");
                    // LOG Error :empty command
                } else if (strcmp(input_buffer, "debug") == 0) {
                    draw_ast_debug_window(debug_window, &tree);
                } else {
                    da_execute(&tree, &a, console_win, input_buffer, &needs_redraw);
                }
                input_len = 0;
                input_buffer[0] = '\0';
                history_index = -1;
            } else if (ch == KEY_UP) {
                if (history_count > 0 && history_index < history_count - 1) {
                    history_index++;
                    strncpy(input_buffer, command_history[history_count - 1 - history_index], 255);
                    input_buffer[255] = '\0';
                    input_len = (int)strlen(input_buffer);
                }
            } else if (ch == KEY_DOWN) {
                if (history_index > 0) {
                    history_index--;
                    strncpy(input_buffer, command_history[history_count - 1 - history_index], 255);
                    input_buffer[255] = '\0';
                    input_len = (int)strlen(input_buffer);
                } else if (history_index == 0) {
                    history_index = -1;
                    input_len = 0;
                    input_buffer[0] = '\0';
                }
            } else if (ch == KEY_RESIZE) {
                // TODO: impement a better resize handling
                getmaxyx(stdscr, screen_height, screen_width);
                needs_redraw = true;

            } else if (ch == KEY_BACKSPACE || ch == 127) {
                if (input_len > 0) {
                    input_buffer[--input_len] = '\0';
                }
            } else if (ch >= 32 && ch <= 126) {
                if (input_len < 255) {
                    input_buffer[input_len++] = ch;
                    input_buffer[input_len] = '\0';
                }
                curs_set(0);
                clear_console_log(console_win);
                curs_set(1);
                // Editing mode
            } else if (ch == '\t') {
                status = Editing;
                last_status = Editing;
                curs_set(0); // UPDATE => hide cursor during move mode, rapid redraws make it flash on Wayland
                int entity_index = 0;
                int relationship_index = 0;
                bool switching_moving_type = true;

                while (switching_moving_type) {
                    int moving_type = getch();
                    if (global_objects.current_dtype == MLD && moving_type == 'r') {
                        moving_type = 'e';
                    }
                    bool moving_relationship = false;
                    bool moving_entity = false;
                    moving = true;
                    switch (moving_type) {
                    case 'e':
                        while (moving) {
                            int move_key = getch();
                            if (move_key == ERR)
                                continue;
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
                            // erase();
                            //  UPDATE => wnoutrefresh(stdscr) instead of refresh() so stdscr and
                            //            console_win flush together in one doupdate inside draw_console_prompt
                            draw_all_and_refresh(screen_width, &moving, &needs_redraw);
                            // TODO : implement a public API to choose whether to make call a draw
                            //  help console or a command console func
                            draw_console_prompt(console_win, input_buffer, status);
                        }
                        break;
                    case 'r':
                        while (moving) {
                            int move_key = getch();
                            if (move_key == ERR)
                                continue;
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
                            // TODO: remove timing profiling before release
                            draw_all_and_refresh(screen_width, &moving, &needs_redraw);
                            draw_console_prompt(console_win, input_buffer, status);
                        }
                        break;

                    case 'x':
                        switching_moving_type = false;
                        status = Typing;
                        last_status = Typing;
                        curs_set(1);         // UPDATE => restore cursor when leaving move mode
                        needs_redraw = true; // TODO: review this (used after setting new time interval and checkinf for
                                             // err for getch())
                        break;
                    }
                }
            }
            break;
        case Help:
            switch (page) {
            case Main:
                // TODO(Critical): make use is it 1 or 0
                curr_main_scrolling_line = 1;
                bool isscrolling = true;

                while (isscrolling) {
                    // UPDATE => use help_win instead of console_win
                    draw_help_window(help_win, &hwin, search_buffer, page, Action, scrolling_pad);
                    int ch = getch();
                    if (ch == ERR) {
                        continue;
                    }
                    switch (ch) {
                    case 'j':
                        curr_main_scrolling_line++;
                        int max_vis_row = get_max_visible_row(curr_main_scrolling_line, Main);
                        if (max_vis_row >= PAD_HOTKEYS_OFFSET - 1) {
                            curr_main_scrolling_line = 1;
                        }
                        break;
                    case 'k':
                        (curr_main_scrolling_line--);
                        break;
                    case 'e':
                        set_current_page(&hwin, Examples);
                        page = Examples;
                        isscrolling = false;
                        curr_main_scrolling_line = 1;
                        break;
                    case 'h':
                        set_current_page(&hwin, Hotkeys);
                        page = Hotkeys;
                        isscrolling = false;
                        curr_main_scrolling_line = 1;
                        break;
                    case '/':
                        // UPDATE => use help_win instead of console_win
                        search_help(help_win, &hwin, search_buffer, search_len, &Action, page, scrolling_pad);
                        break;

                    case KEY_ESCAPE:
                    case 'q':
                        revert_back_to_console(console_win, &status, &needs_redraw);
                    // falls down
                    default:
                        curr_main_scrolling_line = 1;
                        isscrolling = false;
                        break;
                    }
                    set_scrolling_line(&hwin, curr_main_scrolling_line);
                }
                break;

            case Hotkeys:
                curr_hotkey_scrolling_line = 1;
                bool Hotkeys_isscrolling = true;

                while (Hotkeys_isscrolling) {
                    // UPDATE => use help_win instead of console_win
                    draw_help_window(help_win, &hwin, search_buffer, page, Action, scrolling_pad);
                    int ch = getch();
                    if (ch == ERR) {
                        continue;
                    }
                    switch (ch) {
                    case 'j':
                        curr_hotkey_scrolling_line++;
                        int max_vis_row = get_max_visible_row(curr_hotkey_scrolling_line, Hotkeys);
                        if (max_vis_row >= PAD_EXAMPLES_OFFSET - 1) {
                            curr_hotkey_scrolling_line = 1;
                        }
                        break;

                    case 'k':
                        ((curr_hotkey_scrolling_line--) % MAX_LINES_PER_PAGE);
                        break;
                    case 'e':
                        set_current_page(&hwin, Examples);
                        page = Examples;
                        Hotkeys_isscrolling = false;
                        curr_hotkey_scrolling_line = 1;
                        break;
                    case 'm':
                        set_current_page(&hwin, Main);
                        page = Main;
                        Hotkeys_isscrolling = false;
                        curr_hotkey_scrolling_line = 1;
                        break;
                    case '/':
                        // UPDATE => use help_win instead of console_win
                        search_help(help_win, &hwin, search_buffer, search_len, &Action, page, scrolling_pad);
                        break;

                    case 'q':
                        revert_back_to_console(console_win, &status, &needs_redraw);
                    default:
                        curr_hotkey_scrolling_line = 1;
                        Hotkeys_isscrolling = false;
                        break;
                    }
                    set_scrolling_line(&hwin, curr_hotkey_scrolling_line);
                }
                break;

            case Examples:
                curr_examples_scrolling_line = 1;
                bool Examples_isscrolling = true;

                while (Examples_isscrolling) {
                    // UPDATE => use help_win instead of console_win
                    draw_help_window(help_win, &hwin, search_buffer, page, Action, scrolling_pad);
                    int ch = getch();
                    if (ch == ERR) {
                        continue;
                    }
                    switch (ch) {
                    case 'j':
                        curr_examples_scrolling_line++;
                        int max_vis_row = get_max_visible_row(curr_examples_scrolling_line, Examples);
                        if (max_vis_row >= (PAD_EXAMPLES_OFFSET - 1) + MAX_LINES_PER_PAGE) {
                            curr_examples_scrolling_line = 1;
                        }
                        break;
                    case 'k':
                        ((curr_examples_scrolling_line--) % MAX_LINES_PER_PAGE);
                        break;
                    case 'h':
                        curr_examples_scrolling_line = 1;
                        set_current_page(&hwin, Hotkeys);
                        page = Hotkeys;
                        Examples_isscrolling = false;
                        break;
                    case 'm':
                        curr_examples_scrolling_line = 1;
                        set_current_page(&hwin, Main);
                        page = Main;
                        Examples_isscrolling = false;
                        break;
                    case '/':
                        // UPDATE => use help_win instead of console_win
                        search_help(help_win, &hwin, search_buffer, search_len, &Action, page, scrolling_pad);
                        break;

                    case 'q':
                        revert_back_to_console(console_win, &status, &needs_redraw);
                    default:
                        curr_examples_scrolling_line = 1;
                        Examples_isscrolling = false;
                        break;
                    }
                    set_scrolling_line(&hwin, curr_examples_scrolling_line);
                }
                break;
            }
        }
    }

    delwin(console_win);
    // UPDATE => clean up the dedicated help window
    delwin(help_win);
    delwin(scrolling_pad);
    destroy_arena(&a);
    endwin();
    return 0;
}

static void setup_large_e_commerce_delivery_mcd(void) {
    // ==========================================
    // 1. ENTITIES & PROPERTIES
    // (All names strictly < 15 characters)
    // ==========================================

    Entity *customer = createEntity("Customer", 5, 5);
    addProperty(customer, "cust_id", "int", PRIMARY_KEY);
    addProperty(customer, "email", "str", NORMAL_KEY);

    Entity *profile = createEntity("Profile", 50, 5);
    addProperty(profile, "prof_id", "int", PRIMARY_KEY);
    addProperty(profile, "bio", "str", NORMAL_KEY);

    Entity *category = createEntity("Category", 5, 30);
    addProperty(category, "cat_id", "int", PRIMARY_KEY);
    addProperty(category, "cat_name", "str", NORMAL_KEY);

    Entity *order = createEntity("Orders", 50, 30);
    addProperty(order, "order_id", "int", PRIMARY_KEY);
    addProperty(order, "ord_date", "date", NORMAL_KEY);

    Entity *product = createEntity("Product", 95, 30);
    addProperty(product, "prod_id", "int", PRIMARY_KEY);
    addProperty(product, "price", "double", NORMAL_KEY);

    Entity *warehouse = createEntity("Warehouse", 140, 30);
    addProperty(warehouse, "wh_id", "int", PRIMARY_KEY);
    addProperty(warehouse, "capacity", "int", NORMAL_KEY);

    // ==========================================
    // 2. RELATIONSHIPS & CARDINALITIES
    // ==========================================

    // Case A: 1:1 Relationship (Optional on Customer, Mandatory on Profile)
    // MLD EXPECTATION: Profile table gets 'cust_id' as a FOREIGN_KEY (Unique).
    Relationship *r_has_prof = addRelationship(27, 5, customer, profile, "HasProf");
    addCardinalityAPI("0,1,1,1", r_has_prof);

    // Case B: 1:N Relationship (Customer places many Orders)
    // MLD EXPECTATION: Orders table gets 'cust_id' as a FOREIGN_KEY.
    Relationship *r_places = addRelationship(27, 17, customer, order, "Places");
    addCardinalityAPI("0,n,1,1", r_places);

    // Case C: Unary / Reflexive Relationship (Category hierarchy)
    // MLD EXPECTATION: Category gets its own PK as a FOREIGN_KEY (e.g., parent_id).
    // This will severely test your `migrate_foreign_key()` and `references` logic.
    Relationship *r_subcat = addRelationship(5, 50, category, category, "SubCat");
    addCardinalityAPI("0,1,0,n", r_subcat);

    // Case D: N:M Relationship with an Attribute (Order Line)
    // MLD EXPECTATION: A junction table "Contains" is created.
    // It gets 'order_id' and 'prod_id' as FKs (forming a composite PK),
    // AND it must include the 'qty' property as a NORMAL_KEY.
    Relationship *r_contains = addRelationship(72, 30, order, product, "Contains");
    addCardinalityAPI("1,n,0,n", r_contains);
    addPropertyRelationship(r_contains, "qty", "int", NORMAL_KEY);

    // Case E: Standard N:M Relationship (Warehouse Stocks)
    // MLD EXPECTATION: A junction table "Stocks" is created with two FKs.
    Relationship *r_stocks = addRelationship(117, 30, warehouse, product, "Stocks");
    addCardinalityAPI("0,n,0,n", r_stocks);
}
