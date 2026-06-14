#ifndef GRAPHICS_H
#define GRAPHICS_H
#include "DSA/astar.h"
#include "DSA/kmp.h"
#include "MCD_elements.h"
#include "global_objects.h"
#include "help_window.h"
#include <ncurses.h>

#define KEY_ESCAPE 27
//  Windows and scrolling pad
#define CONSOLE_HEIGHT 6 // console_win width = stdscr width
#define HELP_WIN_HEIGHT 22
#define HELP_WIN_WIDTH 82
#define PAD_LINES 60
#define PAD_COLS 78
#define PAD_VIEW_WINDOW 20

#define PAD_OFFSET 17
#define PAD_HOTKEYS_OFFSET 20
#define PAD_EXAMPLES_OFFSET 40
// No need for main offset since it's the first page written in the pad
// Used to traverse to the desired HelpPage ie: Main:0, Examples: Example_line = PAD_OFFSET*Main_line

typedef enum status { Typing, Editing, Help } status;
extern status last_status;

/* Main: Default halp page ie:  basic commands
 * Hotkeys: Keyboard shortcuts ie: Editing mode
 * Examples: Examples duh
 */

void drawEntity(Entity *e);
void drawRelationship(Relationship *r);
void draw_hline_at(int y, int x1, int x2, chtype ch);
void draw_vline_at(int x, int y1, int y2, chtype ch);
void initColors();

// A* FUNCTIONS
void drawConnection(Relationship *r);
void drawConnectionAStar(Relationship *r);
void debugPrintPath(AStarPath *path, const char *name);
AStarPath *smooth_path(AStarPath *path);
void draw_path_with_corners(AStarPath *path);

// DRAW ALL STDSCR
void draw_all_entities(GlobalObjects global_objects, int moving_index, bool is_moving);
void draw_all_relationships(GlobalObjects global_objects, int moving_index, bool is_moving);
void draw_all_and_refresh(int screen_width, bool *moving, bool *needs_redraw);

/* CONSOLE DRAWING FUNCTIONS
 * Console drawing functions call refresh internally
 * The Caller musn't refresh them
 * TODO(Critical): modify to a unified refresh system
 */

WINDOW *create_console_window();
// UPDATE => dedicated help WINDOW so it never shares state with console_win
WINDOW *create_help_window();
void draw_console_prompt(WINDOW *console_win, const char *input, status status);
void draw_help_window(WINDOW *win, HelpWindow *hwin, const char *search_buffer, HelpPage page, HelpAction Action,
                      WINDOW *scrolling_pad);

/* hovering_search_matches is a flag
 * to indicate we are done searching and we are hovering
 * between matches and as a result don't refresh the help_window
 * since it's containing the highlighted results*/

void revert_back_to_console(WINDOW *console_win, status *status, bool *needs_redraw);
void search_help(WINDOW *win, HelpWindow *hwin, char search_buffer[], int search_len, HelpAction *action, HelpPage page,
                 WINDOW *scrolling_pad);
// Pad control (used for scrolling)
WINDOW *init_pad(int num_lines, int num_col, HelpWindow hwin);

// Todo: search why is it  static ? it's private to this file
// and we are including this in main so it's fine for now
static inline int get_max_visible_row(int start_row, int page_type) {
    switch (page_type) {
    case Main:
        return start_row + MAX_LINES_PER_PAGE;
    case Hotkeys:
        return start_row + PAD_HOTKEYS_OFFSET + MAX_LINES_PER_PAGE;
    case Examples:
        return start_row + PAD_EXAMPLES_OFFSET + MAX_LINES_PER_PAGE;
    default:
        return start_row;
    }
}

// Render Search results
void highlight_search_matches(HelpWindow *hwin, WINDOW *win, SearchResult *matches, const char *search_buffer);

#endif
