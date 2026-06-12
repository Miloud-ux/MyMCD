#include "MCD_elements.h"
#include "command_processor.h"
#include "global_objects.h"
#include "graphics.h"
#include "help_window.h"
#include <ncurses.h>
#include <string.h>

// Profiling only

static void setup_large_e_commerce_delivery_mcd(void) {
    Entity *customer = createEntity("Customer", 5, 5);
    addProperty(customer, "customer_id", "int");
    addProperty(customer, "first_name", "str");

    Entity *shopping_cart = createEntity("Cart", 50, 5);
    addProperty(shopping_cart, "cart_id", "int");

    Entity *order = createEntity("Order", 95, 5);
    addProperty(order, "order_id", "int");

    Entity *product = createEntity("Product", 140, 5);
    addProperty(product, "product_id", "int");

    Entity *delivery = createEntity("Delivery", 5, 27);
    addProperty(delivery, "track_num", "str");

    Entity *warehouse = createEntity("Warehouse", 95, 27);
    addProperty(warehouse, "capacity", "int");

    Relationship *r_owns = addRelationship(31, 5, customer, shopping_cart, "Owns");
    addCardinalityAPI("1,1,0,1", r_owns);

    Relationship *r_checkout = addRelationship(76, 5, shopping_cart, order, "Checkout");
    addCardinalityAPI("0,1,1,1", r_checkout);

    Relationship *r_order_line = addRelationship(121, 5, order, product, "Ord_Line");
    addCardinalityAPI("1,n,0,n", r_order_line);

    Relationship *r_fulfilled = addRelationship(25, 16, order, delivery, "ShipsVia");
    addCardinalityAPI("0,1,1,1", r_fulfilled);

    Relationship *r_stocks = addRelationship(121, 16, warehouse, product, "Stocks");
    addCardinalityAPI("1,n,0,n", r_stocks);
}

int main() {
    initscr();
    // =>>

    initColors();

    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // TODO: move to graphics.h header
    enum fps { SIXTY = 16, THIRTY = 30 }; // 60, 30 fps
    enum fps update_interval = SIXTY;
    timeout(update_interval);

    int screen_height, screen_width;
    getmaxyx(stdscr, screen_height, screen_width);

    init_global_objects();
    // test diagram

    // == WINDOWS ==
    WINDOW *console_win = create_console_window();
    WINDOW *help_win = create_help_window();
    char input_buffer[256] = "";
    int input_len = 0;
    char search_buffer[MAX_SEARCH_BUFFER_LEN] = "";
    int search_len = 0;
    HelpAction Action = Navigation;

    // Stdscreen info
    int std_screen_width, std_screen_height;
    getmaxyx(stdscr, std_screen_height, std_screen_width);

    // Help window info
    int center_y = (std_screen_height - HELP_WIN_HEIGHT) / 2;
    int center_x = (std_screen_width - HELP_WIN_WIDTH) / 2;
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

    // Drawing
    setup_large_e_commerce_delivery_mcd();
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
                if (strcmp(input_buffer, "exit") == 0 || strcmp(input_buffer, "quit") == 0) {
                    is_running = false;
                } else if (strcmp(input_buffer, "help") == 0) {
                    status = Help;
                    last_status = Help;
                    curs_set(0); // UPDATE => hide cursor while help is open, cursor warping is the flicker
                } else if (strlen(input_buffer) == 0) {
                    // mvwprintw(stdscr, screen_height / 2, screen_width / 2, "UPDATED");
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
                last_status = Editing;
                curs_set(0); // UPDATE => hide cursor during move mode, rapid redraws make it flash on Wayland
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
    endwin();
    return 0;
}
