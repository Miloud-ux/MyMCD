#include "command_processor.h"
#include "Lexer/parse.h"
#include "Lexer/tokenize.h"
#include <assert.h>
#include <ctype.h>
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

bool da_execute(AST *t, Arena *a, WINDOW *console_win, const char *input, bool *needs_redraw) {
    if (!t) {
        // LOG: failed to build AST (Dosn't  exist)
        return false;
    }

    TempArena temp_arena = init_temp_arena(a);
    Parser *p = ARENA_PUSH_OBJECT(a, Parser);
    assert(p != NULL);
    init_parser(p, input);
    tokenize_content(input, p->tokens, &p->count);

    /* Why is parse_command returning AST* ptr:
     * To avoid loosing AST changes when
     * freeing temp arena we return a pointer
     * to it so that after freeing temp_arena
     * we can commit the changes and push to the
     * real arena
     */

    parse_command(t, p, input, console_win);
    *needs_redraw = true;
    free_temp_arena(temp_arena);

    return true;
}
