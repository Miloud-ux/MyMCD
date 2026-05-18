#include "help_window.h"

static HelpPageData helpdb[HelpPageNum] = {
    [Main] = {.lines = {{.text = "Entity is a box"}, {.text = "Property is a field"}}, .line_count = 2},
    [Hotkeys] = {.lines = {{.text = "Press arrow up"}, {.text = "Press / to search"}}, .line_count = 2},
    [Examples] = {.lines = {{.text = "Create relationship"}, {.text = "Create entity"}}, .line_count = 2}};

void init_help_window(HelpWindow *win, HelpPage current_page) {
    win->top_visible_line = 0;
    win->current_page = current_page;
    win->pages_db = helpdb;

    for (int p = 0; p < HelpPageNum; p++) {
        for (size_t l = 0; l < helpdb[p].line_count; l++) {
            HelpLine *current_line = (HelpLine *)&win->pages_db[p].lines[l];
            if (current_line->text != NULL) {
                // tokenize
                tokenize_content(current_line->text, current_line->tokens, (int *)&current_line->token_count);
            }
        }
    }
}

void set_current_page(HelpWindow *win, HelpPage page);
