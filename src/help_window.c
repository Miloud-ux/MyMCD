#include "help_window.h"

static HelpPageData helpdb[HelpPageNum] =
    {[Main] =
         {.lines =
              {{.text = "=== MCD Designer Help CLI ===", .line_start = 1, .type = h},
               {.text = "This utility builds entity-relationship models using MCD rules.", .line_start = 2, .type = p},
               {.text = "", .line_start = 3, .type = p},
               {.text = "CORE ARCHITECTURE CONCEPTS:", .line_start = 4, .type = h1},
               {.text = "  * ENTITY: Object class holding unique properties and an Identifier.",
                .line_start = 5,
                .type = p},
               {.text = "  * RELATIONSHIP: Links entities to symbolize physical associations.",
                .line_start = 6,
                .type = p},
               {.text = "  * CARDINALITY: Specifies min/max occurrences (0,1 / 1,1 / 0,N / 1,N).",
                .line_start = 7,
                .type = p},
               {.text = "", .line_start = 8, .type = p},
               {.text = "RELATIONSHIP STYLES:", .line_start = 9, .type = h1},
               {.text = "  * BINARY RELATIONSHIP: Standard link connecting two distinct entities.",
                .line_start =
                    10,
                .type =
                    p},
               {.text = "  * REFLEXIVE (SINGLE): An entity linking directly to itself.", .line_start = 11, .type = p},
               {.text = "  * TERNARY / N-ARY: A link connecting three or more entities.", .line_start = 12, .type = p},
               {.text = "", .line_start = 13, .type = p},
               {.text = "CLI PROMPT SYNTAX ENGINE:", .line_start = 14, .type = h1},
               {.text = "All explicit adjustments use the command line panel.", .line_start = 15, .type = p},
               {.text = "", .line_start = 16, .type = p},
               {.text = "MCD TRIVIA: Peter Chen introduced the original ER syntax in 1976.", .line_start = 17, .type = p},
               {.text = "MERISE TRIVIA: Merise introduced the modern MCD system in Europe.", .line_start = 18, .type = p},
               {.text = "", .line_start = 19, .type = p},
               {.text = "Press 'e' for examples                 Press 'h' for hotkeys", .line_start = 20, .type = hint}},
          .line_count = 20},
     [Hotkeys] = {.lines = {{.text = "==== HOTKEYS & NAVIGATION ====", .line_start = 21, .type = h},
                            {.text = "Use these parameters to alter active nodes on your canvas layout.",
                             .line_start = 22,
                             .type = p},
                            {.text = "", .line_start = 23, .type = p},
                            {.text = "INTERFACE CONTROLS:", .line_start = 24, .type = h1},
                            {.text = "  [TAB]        Enter General Edit Mode", .line_start = 25, .type = p},
                            {.text = "  [e]          Move Entities (use Arrow Keys to shift box location)",
                             .line_start = 26,
                             .type = p},
                            {.text = "  [r]          Move Relationships (adjust link nodes across canvas)",
                             .line_start = 27,
                             .type = p},
                            {.text = "  [q]          Back to General Edit / Exit Page", .line_start = 28, .type = p},
                            {.text = "  [x]          Quit Movement Mode safely", .line_start = 29, .type = p},
                            {.text = "", .line_start = 30, .type = p},
                            {.text = "MCD ARCHITECTURE RULES:", .line_start = 31, .type = h1},
                            {.text = "  * An entity name must always remain unique inside the model space.",
                             .line_start = 32,
                             .type = p},
                            {.text = "  * Relationships cannot link directly to other relationship blocks.",
                             .line_start = 33,
                             .type = p},
                            {.text = "", .line_start = 34, .type = p},
                            {.text = "NCURSES TRIVIA:", .line_start = 35, .type = h1},
                            {.text = "  * Ncurses handles window layers using custom virtual screen pads.",
                             .line_start = 36,
                             .type = p},
                            {.text = "  * A 1,N cardinality means a record matches at least one item.",
                             .line_start = 37,
                             .type = p},
                            {.text = "  * Identifiers map directly into relational columns downstream.",
                             .line_start = 38,
                             .type = p},
                            {.text = "", .line_start = 39, .type = p},
                            {.text = "Press 'e' for examples                 Press 'm' back to main help",
                             .line_start = 40,
                             .type = hint}},
                  .line_count = 20},
     [Examples] = {
         .lines = {{.text = "==== COMMAND SYNTAX & EXAMPLES ====", .line_start = 41, .type = h},
                   {.text = "Execute model assembly behaviors using these structural actions.",
                    .line_start = 42,
                    .type = p},
                   {.text = "", .line_start = 43, .type = p},
                   {.text = "CORE COMMAND UTILITIES:", .line_start = 44, .type = h1},
                   {.text = ">> create entity \"e_name\"", .line_start = 45, .type = code},
                   {.text = ">> create relationship \"r_name\" \"e1_name\" \"e2_name\"", .line_start = 46, .type = code},
                   {.text = ">> add property \"target\" \"prop\" type", .line_start = 47, .type = code},
                   {.text = "", .line_start = 48, .type = p},
                   {.text = "SYNTAX CONSTRAINTS:", .line_start = 49, .type = h1},
                   {.text = "Note: You MUST use quotations either double or single around names.",
                    .line_start = 50,
                    .type = p},
                   {.text = "", .line_start = 51, .type = p},
                   {.text = "MCD MODEL SCENARIOS:", .line_start = 52, .type = h1},
                   {.text = "  * Binary: Entity \"User\" links via \"Orders\" to Entity \"Product\".",
                    .line_start = 53,
                    .type = p},
                   {.text = "  * Cardinality: User (1,N) ---- Orders ---- (0,1) Product.", .line_start = 54, .type = p},
                   {.text = "  * Reflexive: \"Employee\" linked via \"Manages\" to \"Employee\".",
                    .line_start = 55,
                    .type = p},
                   {.text = "", .line_start = 56, .type = p},
                   {.text = "DATABASE DESIGN TRIVIA:", .line_start = 57, .type = h1},
                   {.text = "  * Normalizing schemas prevents unexpected database deletion bugs.",
                    .line_start = 58,
                    .type = p},
                   {.text = "", .line_start = 59, .type = p},
                   {.text = "Press 'h' for hotkeys                  Press 'm' back to main help",
                    .line_start = 60,
                    .type = hint}},
         .line_count = 20}};

// Functions

void init_help_window(HelpWindow *win, HelpPage current_page) {
    win->current_page = current_page;
    win->pages_db = helpdb;

    for (int p = 0; p < HelpPageNum; p++) {
        for (size_t l = 0; l < helpdb[p].line_count; l++) {
            HelpLine *current_line = (HelpLine *)&win->pages_db[p].lines[l];
            if (current_line->text != NULL) {
                // tokenize
                tokenize_content(current_line->text, current_line->tokens, (int *)&current_line->token_count);
                current_line->line_len = current_line->tokens[current_line->token_count].pos;
            }
        }
    }
}

void set_current_page(HelpWindow *win, HelpPage page) {
    if (!win) {
        return;
    }
    win->current_page = page;
}
