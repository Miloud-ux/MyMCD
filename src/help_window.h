#ifndef HELP_WINDOW_H
#define HELP_WINDOW_H

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

#define HELP_WIN_HEIGHT 22
#define HELP_WIN_WIDTH 80

#define MAX_LINES_PER_PAGE 3
#define MAX_DIALOGE_LEN 80 // equal to the width of the help window
#define MAX_TOKENS_PER_LINE 40

typedef enum HelpPage { Main, Hotkeys, Examples, HelpPageNum } HelpPage;
// Main: default page

typedef struct HelpLine {
        const char *text;
        Token tokens[MAX_TOKENS_PER_LINE];
        size_t token_count;
} HelpLine;

typedef struct HelpPageData {
        HelpLine lines[MAX_LINES_PER_PAGE];
        size_t line_count;
} HelpPageData;

typedef struct HelpWindow {
        HelpPage current_page;
        int top_visible_line; // for scrolling
        const HelpPageData *pages_db;
} HelpWindow;

void init_help_window(HelpWindow *win, HelpPage current_page);
void set_current_page(HelpWindow *win, HelpPage page);
#endif // !HELP_WINDOW_H
