#include "help_window.h"
#include "graphics.h"

static HelpPageData helpdb[HelpPageNum] =
    {[Main] =
         {.lines =
              {{.text = "=== MCD Designer Help CLI ===", .line_start = 1, .type = h},
               {.text = "Model your data as Entities and Relationships (MCD), then convert it to a logical schema (MLD).",
                .line_start = 2,
                .type = p},
               {.text = "", .line_start = 3, .type = p},
               {.text = "CORE CONCEPTS:", .line_start = 4, .type = h1},
               {.text = "  * ENTITY: object with named properties; exactly one should be the primary key (pk).",
                .line_start = 5,
                .type = p},
               {.text = "  * RELATIONSHIP: connects two entities and may carry its own properties.",
                .line_start = 6,
                .type = p},
               {.text = "  * CARDINALITY \"min,max\": occurrences allowed on a side, e.g. \"1,n\" or \"0,1\".",
                .line_start = 7,
                .type = p},
               {.text = "", .line_start = 8, .type = p},
               {.text = "HOW 'convert MLD' DECIDES THE SCHEMA:", .line_start = 9, .type = h1},
               {.text = "  * max \"n,n\"           -> creates a junction entity holding both sides' pk's.",
                .line_start = 10,
                .type = p},
               {.text = "  * max \"1,n\"           -> the \"1\" side's pk migrates as a foreign key into the \"n\" side.",
                .line_start = 11,
                .type = p},
               {.text = "  * max \"1,1\" both ways -> the side whose min cardinality is \"1\" gets the foreign key.",
                .line_start = 12,
                .type = p},
               {.text = "  * Name clash (e.g. two \"id\") -> migrated property is renamed \"<source>_<name>\".",
                .line_start = 13,
                .type =
                    p},
               {.text = "", .line_start = 14, .type = p},
               {.text = "QUICK NAVIGATION:", .line_start = 15, .type = h1},
               {.text = "All commands use quotes: create, add, change name, delete, convert, save, clear.",
                .line_start = 16,
                .type =
                    p},
               {.text = "Press 'e' for examples                 Press 'h' for hotkeys", .line_start = 17, .type = hint}},
          .line_count = 17},
     [Hotkeys] =
         {.lines = {{.text = "==== HOTKEYS & NAVIGATION ====", .line_start = 21, .type = h},
                    {.text = "These keys control the canvas (not the command line) while editing your diagram.",
                     .line_start = 22,
                     .type = p},
                    {.text = "", .line_start = 23, .type = p},
                    {.text = "INTERFACE CONTROLS:", .line_start = 24, .type = h1},
                    {.text = "  [TAB]   Enter General Edit Mode (choose what to move)", .line_start = 25, .type = p},
                    {.text = "  [e]     Move Entities      (arrow keys shift the selected box)",
                     .line_start = 26,
                     .type = p},
                    {.text = "  [r]     Move Relationships (arrow keys shift the selected diamond)",
                     .line_start = 27,
                     .type = p},
                    {.text = "  [x]     Stop moving, back to General Edit Mode", .line_start = 28, .type = p},
                    {.text = "  [q]     Leave the current mode / close this help page", .line_start = 29, .type = p},
                    {.text = "", .line_start = 30, .type = p},
                    {.text = "MCD RULES ENFORCED BY THE PARSER:", .line_start = 31, .type = h1},
                    {.text = "  * Entity and relationship names must be unique across the whole diagram.",
                     .line_start = 32,
                     .type = p},
                    {.text = "  * A relationship can only link entities, never another relationship.",
                     .line_start = 33,
                     .type = p},
                    {.text = "  * Foreign keys ('fk') are rejected on an MCD; convert to MLD first to add them.",
                     .line_start = 34,
                     .type = p},
                    {.text = "  * Deleting an entity also deletes any relationship still attached to it.",
                     .line_start = 35,
                     .type = p},
                    {.text = "", .line_start = 36, .type = p},
                    {.text = "Press 'e' for examples                 Press 'm' back to main help",
                     .line_start = 37,
                     .type = hint}},
          .line_count = 17},
     [Examples] = {
         .lines = {{.text = "==== COMMAND SYNTAX & EXAMPLES ====", .line_start = 41, .type = h},
                   {.text = "Quotes are required around every name, filename, and cardinality value.",
                    .line_start = 42,
                    .type = p},
                   {.text = "CREATE / RENAME / DELETE:", .line_start = 43, .type = h1},
                   {.text = ">> create entity \"Student\"", .line_start = 44, .type = code},
                   {.text = ">> create relationship \"Enrolls\" \"Student\" \"Course\"", .line_start = 45, .type = code},
                   {.text = ">> change name \"Student\" \"Pupil\"", .line_start = 46, .type = code},
                   {.text = ">> delete \"Pupil\"", .line_start = 47, .type = code},
                   {.text = "PROPERTIES (types: int str double date money, keys: pk fk):", .line_start = 48, .type = h1},
                   {.text = ">> add property \"Student\" \"gpa\" double pk", .line_start = 49, .type = code},
                   {.text = ">> add property \"Enrolls\" \"grade\" str", .line_start = 50, .type = code},
                   {.text = "CARDINALITY (\"min,max\"):", .line_start = 51, .type = h1},
                   {.text = ">> add card \"Enrolls\" \"1,n,0,n\"", .line_start = 52, .type = code},
                   {.text = ">> add card \"Enrolls\" \"Student\" \"1,n\"", .line_start = 53, .type = code},
                   {.text = "CONVERT & SAVE:", .line_start = 54, .type = h1},
                   {.text = ">> convert MLD", .line_start = 55, .type = code},
                   {.text = ">> save MLD \"diagram.txt\"", .line_start = 56, .type = code},
                   {.text = "Press 'h' for hotkeys                  Press 'm' back to main help",
                    .line_start = 57,
                    .type = hint}},
         .line_count = 17}};

// Functions
void init_help_window(HelpWindow *win, HelpPage current_page) {
    win->current_page = current_page;
    win->pages_db = helpdb;
    win->examples_scrolling_line = PAD_OFFSET * 2;
    win->main_scrolling_line = 0;
    win->hotkey_scrolling_line = PAD_OFFSET;

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

void set_scrolling_line(HelpWindow *win, int line) {
    if (line < 0) {
        return;
    }
    switch (win->current_page) {
    case Main:
        win->main_scrolling_line = line;
        break;

    case Hotkeys:
        win->hotkey_scrolling_line = line;
        break;

    case Examples:
        win->examples_scrolling_line = line;
        break;
    }
}
