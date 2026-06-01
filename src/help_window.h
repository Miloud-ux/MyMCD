#pragma once
/* This header is used to contain all the functions
 * related to the help win and the structs/enums
 * related to it
 */

/* ===Lines Numbers problem===
 * Dialogue[] indices can be used to determine line number like this
 * firstLineNum = LineNumbers[0]; This is where the first line of help dialogue starts
 * nthLineNum = firstLineNum + indexof(Dialogue[i])
 */

#include "Lexer/tokenize.h"
#include <stddef.h>

#define MAX_LINES_PER_PAGE 17
#define MAX_TOKENS_PER_LINE 80
#define MAX_SEARCH_BUFFER_LEN 256

typedef enum LineType {
    // TODO: implement Better Rendering
    p,    // regular text
    h,    // only for the first scroll ( top most title )
    h1,   //  HTML like
    hint, // hint the bottom most text ie: press e to go to examples...etc
    code  // for showcasing commands like : create
} LineType;
typedef enum HelpPage { Main, Hotkeys, Examples, HelpPageNum } HelpPage;
typedef enum HelpAction {
    Navigation, // Defaults to 0
    Search      // Search = 1 (true)
} HelpAction;

// Main: default page

typedef struct HelpLine {
        const char *text;
        Token tokens[MAX_TOKENS_PER_LINE];
        size_t token_count;
        size_t line_len; // equal to the pos of the last token
        LineType type;
        int line_start; // position on the page
} HelpLine;

typedef struct HelpPageData {
        HelpLine lines[MAX_LINES_PER_PAGE];
        size_t line_count;
} HelpPageData;

typedef struct HelpWindow {
        HelpPage current_page;
        const HelpPageData *pages_db;
        int main_scrolling_line;
        int hotkey_scrolling_line;
        int examples_scrolling_line;
} HelpWindow;

void init_help_window(HelpWindow *win, HelpPage current_page);
void set_current_page(HelpWindow *win, HelpPage page);
void set_scrolling_line(HelpWindow *win, int line);
