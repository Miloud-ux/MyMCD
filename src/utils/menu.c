// DRAWING STRATEGY:
//   The menu window is sized to fit its content (fixed at MENU_W x MENU_H
//   characters) and is placed at the exact centre of stdscr each time
//   show_startup_menu() is called.  This means it works correctly even if
//   the terminal has been resized before launch.
//
//   Layout inside the box (coordinates are window-relative):
//
//     Row 0   : box top border (drawn by box())
//     Row 1   : "[ MCD Tool ]" centred, bold
//     Row 2   : separator line  (ACS_HLINE across the full inner width)
//     Row 3   : blank padding
//     Row 4   : option 1
//     Row 5   : option 2
//     Row 6   : option 3
//     Row 7   : blank padding
//     Row 8   : hint text "Use arrows + ENTER"  (dim)
//     Row 9   : box bottom border
//
// BLINKING / HIGHLIGHT:
//   The selected option is printed with wattron(win, A_BLINK | A_REVERSE).
//   A_BLINK is a standard ncurses attribute; whether it actually causes
//   visible blinking depends on the terminal emulator and TERM setting.
//   On terminals that do not support blinking (e.g. most VTE-based terminals
//   with blink disabled) the cell still renders with reverse video, which is
//   clearly visible.  No workaround is used because adding a software blink
//   loop would require either a secondary thread or tight polling in getch()
//   with timeout(), both of which introduce complexity that the rest of the
//   codebase deliberately avoids.
//
// INPUT LOOP:
//   getch() is used in blocking mode (nodelay is NOT set here; the main loop
//   sets it later).  The function returns as soon as the user presses ENTER
//   or a digit shortcut.  KEY_RESIZE is handled by a full redraw.
//
// COLOR USAGE:
//   The title uses COLOR_PAIR(2) (green on black, the same "success" colour
//   used elsewhere in the UI) so the branding feels consistent.  The hint
//   text uses A_DIM.

#include "menu.h"

#define MENU_H 12
#define MENU_W 46

#define NUM_OPTIONS 3

static const char *OPTION_LABELS[NUM_OPTIONS] = {
    "  [1]  New Diagram  ",
    "  [2]  Load Diagram ",
    "  [3]  Exit         ",
};

static void draw_menu(WINDOW *win, int selected, int win_h, int win_w) {
    (void)win_h; // suppress unused-warning; reserved for future dynamic sizing

    werase(win);
    box(win, 0, 0);

    // Title
    const char *title = "MyMCD";
    int title_x = (win_w - (int)strlen(title)) / 2;
    wattron(win, A_BOLD | COLOR_PAIR(2));
    mvwprintw(win, 1, title_x, "%s", title);
    wattroff(win, A_BOLD | COLOR_PAIR(2));

    // Separator line below the title
    wmove(win, 2, 1);
    whline(win, ACS_HLINE, win_w - 2);
    // Reconnect the separator to the side borders
    mvwaddch(win, 2, 0, ACS_LTEE);
    mvwaddch(win, 2, win_w - 1, ACS_RTEE);

    // Options
    for (int i = 0; i < NUM_OPTIONS; i++) {
        int row = 4 + i;
        int label_len = (int)strlen(OPTION_LABELS[i]);
        int col = (win_w - label_len) / 2;

        if (i == selected) {
            wattron(win, A_BLINK | A_REVERSE);
            mvwprintw(win, row, col, "%s", OPTION_LABELS[i]);
            wattroff(win, A_BLINK | A_REVERSE);
        } else {
            mvwprintw(win, row, col, "%s", OPTION_LABELS[i]);
        }
    }

    // Hint text
    const char *hint = "arrows / number + ENTER to select";
    int hint_x = (win_w - (int)strlen(hint)) / 2;
    wattron(win, A_DIM);
    mvwprintw(win, 8, hint_x, "%s", hint);
    wattroff(win, A_DIM);

    wattron(win, COLOR_PAIR(3));

    const char *credit = "Made by Miloud";
    int credit_x = (win_w - (int)strlen(credit)) / 2;
    mvwprintw(win, 10, credit_x, "Made by Miloud");
    wattroff(win, COLOR_PAIR(3));

    wrefresh(win);
}

MenuChoice show_startup_menu(void) {
    curs_set(0); // hide cursor during menu
    int selected = 0;

    int scr_h, scr_w;
    getmaxyx(stdscr, scr_h, scr_w);

    int start_y = (scr_h - MENU_H) / 2;
    int start_x = (scr_w - MENU_W) / 2;
    if (start_y < 0)
        start_y = 0;
    if (start_x < 0)
        start_x = 0;

    WINDOW *win = newwin(MENU_H, MENU_W, start_y, start_x);
    keypad(win, TRUE);

    draw_menu(win, selected, MENU_H, MENU_W);

    while (1) {
        int ch = wgetch(win);

        switch (ch) {
        case KEY_UP:
            selected = (selected - 1 + NUM_OPTIONS) % NUM_OPTIONS;
            break;

        case KEY_DOWN:
            selected = (selected + 1) % NUM_OPTIONS;
            break;

        case '1':
            selected = 0;
            draw_menu(win, selected, MENU_H, MENU_W);
            // fall through to confirm
            goto confirm;

        case '2':
            selected = 1;
            draw_menu(win, selected, MENU_H, MENU_W);
            goto confirm;

        case '3':
            selected = 2;
            draw_menu(win, selected, MENU_H, MENU_W);
            goto confirm;

        case '\n':
        case '\r':
        case KEY_ENTER:
        confirm:
            delwin(win);
            curs_set(1);
            clear();
            refresh();
            return (MenuChoice)selected;

        case KEY_RESIZE: {
            // Recentre on terminal resize
            getmaxyx(stdscr, scr_h, scr_w);
            start_y = (scr_h - MENU_H) / 2;
            start_x = (scr_w - MENU_W) / 2;
            if (start_y < 0)
                start_y = 0;
            if (start_x < 0)
                start_x = 0;
            mvwin(win, start_y, start_x);
            break;
        }

        default:
            break;
        }

        draw_menu(win, selected, MENU_H, MENU_W);
    }
}
