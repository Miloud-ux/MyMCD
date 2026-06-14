#include "command_processor.h"
#include "Lexer/parse.h"
#include "Lexer/tokenize.h"
#include "MCD_elements.h"
#include "global_objects.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static void to_lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

void show_message(WINDOW *console_win, const char *msg) {
    wmove(console_win, 1, 1);
    wclrtoeol(console_win);
    mvwprintw(console_win, 1, 1, ">> %s", msg);
    wrefresh(console_win);
}

void da_execute(Arena *a, WINDOW *console_win, const char *input, bool *needs_redraw) {
    TempArena t = init_temp_arena(a);
    Parser *p = ARENA_PUSH_OBJECT(a, Parser);
    assert(p != NULL);
    init_parser(p, input);
    tokenize_content(input, p->tokens, &p->count);
    parse_command(p, input, console_win);
    *needs_redraw = true;
    free_temp_arena(t);
}
