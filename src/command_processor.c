#include "command_processor.h"
#include "Lexer/parse.h"
#include "Lexer/tokenize.h"
#include <assert.h>
#include <ctype.h>

void show_message(WINDOW *console_win, const char *msg) {
    wmove(console_win, 1, 1);
    wclrtoeol(console_win);
    mvwprintw(console_win, 1, 1, ">> %s", msg);
    wrefresh(console_win);
}

bool execute_command(AST *t, Arena *a, WINDOW *console_win, const char *input, bool *needs_redraw) {
    if (!t) {
        // LOG: failed to build AST (Dosn't  exist)
        return false;
    }

    TempArena temp_arena = init_temp_arena(a);
    Parser p; // Keep the parser on the stack
    init_parser(&p, input);
    tokenize_content(input, p.tokens, &p.count);

    if (parse_command(t, &p, input, console_win, a)) {

        *needs_redraw = true;
        return true;
    } else {
        free_temp_arena(temp_arena);
        return false;
    }
}
