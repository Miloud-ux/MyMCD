#include "save.h"
#include "../command_processor.h"
#include "../global_objects.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool confirm_override(WINDOW *console_win, const char *filename) {
    wmove(console_win, 1, 1);
    wclrtoeol(console_win);
    wmove(console_win, 2, 1);
    wclrtoeol(console_win);

    wattron(console_win, COLOR_PAIR(3));
    mvwprintw(console_win, 1, 1, "[WARNING]: ");
    wattroff(console_win, COLOR_PAIR(3));
    mvwprintw(console_win, 1, 12, "File '%s' already exists. Override? (y/n)", filename);
    mvwprintw(console_win, 2, 1, "> ");
    wrefresh(console_win);

    nodelay(stdscr, FALSE);
    int ch = getch();
    nodelay(stdscr, TRUE);

    wmove(console_win, 2, 1);
    wclrtoeol(console_win);
    wrefresh(console_win);

    return (ch == 'y' || ch == 'Y');
}

static const char *key_type_str(KeyType k) {
    switch (k) {
    case PRIMARY_KEY:
        return " pk";
    case FOREIGN_KEY:
        return " fk";
    default:
        return "";
    }
}

bool save_diagram(const char *filename, DiagramType diagram_type, WINDOW *console_win) {
    if (!filename || filename[0] == '\0') {
        show_message(console_win, "Save failed: no filename given");
        return false;
    }

    FILE *probe = fopen(filename, "r");
    if (probe) {
        fclose(probe);
        if (!confirm_override(console_win, filename)) {
            show_message(console_win, "Save cancelled");
            return false;
        }
    }

    FILE *f = fopen(filename, "w");
    if (!f) {
        show_message(console_win, "Save failed: could not open file for writing");
        return false;
    }

    const char *dtype_str = (diagram_type == MLD) ? "MLD" : "MCD";
    fprintf(f, "# %s\n", dtype_str);

    if (diagram_type == MLD) {
        fprintf(f, "convert MLD;\n");
    }

    for (int i = 0; i < global_objects.entity_count; i++) {
        Entity *e = global_objects.entities[i];
        if (!e)
            continue;

        fprintf(f, "create entity \"%s\";\n", e->name);

        for (int j = 0; j < e->num_properties; j++) {
            Property *p = e->properties[j];
            if (!p)
                continue;
            fprintf(f, "add property \"%s\" \"%s\" %s%s;\n", e->name, p->name, p->type, key_type_str(p->keytype));
        }
    }

    for (int i = 0; i < global_objects.relationship_count; i++) {
        Relationship *r = global_objects.relationships[i];
        if (!r)
            continue;
        if (!r->e1 || !r->e2)
            continue;

        fprintf(f, "create relationship \"%s\" \"%s\" \"%s\";\n", r->name, r->e1->name, r->e2->name);

        // Properties on the relationship
        for (int j = 0; j < r->num_properties; j++) {
            Property *p = r->properties[j];
            if (!p)
                continue;
            fprintf(f, "add property \"%s\" \"%s\" %s%s;\n", r->name, p->name, p->type, key_type_str(p->keytype));
        }

        // Cardinality: reconstruct the "min1,max1,min2,max2" raw string
        if (r->cards[0] && r->cards[1]) {
            // cards[n]->value is stored as "x,y\0" (CARDINALITY_LEN = 4)
            // We need to combine them into one "x,y,x,y" raw string
            char raw[RAW_CARDINALITY_LEN]; // 9 bytes: "x,y,x,y\0"
            snprintf(raw, RAW_CARDINALITY_LEN, "%c,%c,%c,%c", r->cards[0]->value[0], r->cards[0]->value[2],
                     r->cards[1]->value[0], r->cards[1]->value[2]);
            fprintf(f, "add card \"%s\" \"%s\";\n", r->name, raw);
        }
    }

    fclose(f);

    char msg[128];
    snprintf(msg, sizeof(msg), "Diagram saved to '%s'", filename);
    show_message(console_win, msg);
    return true;
}

bool load_diagram(const char *filename, Arena *a, AST *tree, WINDOW *console_win, bool *needs_redraw) {
    if (!filename || filename[0] == '\0') {
        show_message(console_win, "Load failed: no filename given");
        return false;
    }

    FILE *f = fopen(filename, "r");
    if (!f) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Load failed: could not open '%s'", filename);
        show_message(console_win, msg);
        return false;
    }

    //  file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(f);
        show_message(console_win, "Load failed: file is empty");
        return false;
    }

    // slurp into arena buffer (+1 for null terminator)
    char *buf = (char *)arena_alloc_aligned(a, (size_t)(file_size + 1), DEFAULT_ALIGNMENT);
    if (!buf) {
        fclose(f);
        show_message(console_win, "Load failed: out of arena memory");
        return false;
    }

    size_t bytes_read = fread(buf, 1, (size_t)file_size, f);
    fclose(f);
    buf[bytes_read] = '\0';

    // walk buffer, splitting on ';'
    // we parse in-place using a write pointer to build each command string.
    bool any_ok = false;
    char *p = buf;

    while (*p != '\0') {
        // trim
        while (*p != '\0' && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
            p++;
        }
        if (*p == '\0')
            break;

        // skip comment lines (lines starting with '#')
        if (*p == '#') {
            while (*p != '\0' && *p != '\n')
                p++;
            continue;
        }

        // find the end of this command (';' delimiter)
        char *cmd_start = p;
        char *semicolon = cmd_start;
        while (*semicolon != '\0' && *semicolon != ';') {
            semicolon++;
        }

        if (*semicolon == '\0') {
            // No semicolon found either end of file or malformed last line.
            // attempt to execute whatever is left if non-empty.
            if (semicolon > cmd_start) {
                *semicolon = '\0';
                // trim trailing whitespace
                char *end = semicolon - 1;
                while (end > cmd_start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
                    *end-- = '\0';
                }
                if (*cmd_start != '\0') {
                    if (execute_command(tree, a, console_win, cmd_start, needs_redraw)) {
                        any_ok = true;
                    }
                }
            }
            break;
        }

        *semicolon = '\0';
        char *end = semicolon - 1;
        while (end > cmd_start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
            *end-- = '\0';
        }

        // execute command if non-empty
        if (*cmd_start != '\0') {
            if (execute_command(tree, a, console_win, cmd_start, needs_redraw)) {
                any_ok = true;
            }
        }

        p = semicolon + 1; // advance past the '\0' we wrote over ';'
    }

    if (any_ok) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Diagram loaded from '%s'", filename);
        show_message(console_win, msg);
    } else {
        show_message(console_win, "Load finished but no commands executed (empty or all errors)");
    }

    return any_ok;
}

size_t snapshot_diagram_to_buf(char *buf, size_t buf_size) {
    if (!buf || buf_size == 0) {
        return 0;
    }

    size_t pos = 0;

// helper macro: appends a formatted string to buf, advancing pos.
// stops writing if the buffer is full but keeps counting so the caller
// can detect truncation (pos >= buf_size).
#define SNAP_WRITE(...)                                                                                                  \
    do {                                                                                                                 \
        if (pos < buf_size) {                                                                                            \
            int _n = snprintf(buf + pos, buf_size - pos, __VA_ARGS__);                                                   \
            if (_n > 0)                                                                                                  \
                pos += (size_t)_n;                                                                                       \
        }                                                                                                                \
    } while (0)

    DiagramType dtype = global_objects.current_dtype;
    SNAP_WRITE("# %s\n", (dtype == MLD) ? "MLD" : "MCD");

    if (dtype == MLD) {
        SNAP_WRITE("convert MLD;\n");
    }

    for (int i = 0; i < global_objects.entity_count; i++) {
        Entity *e = global_objects.entities[i];
        if (!e)
            continue;

        SNAP_WRITE("create entity \"%s\";\n", e->name);

        for (int j = 0; j < e->num_properties; j++) {
            Property *p = e->properties[j];
            if (!p)
                continue;
            SNAP_WRITE("add property \"%s\" \"%s\" %s%s;\n", e->name, p->name, p->type, key_type_str(p->keytype));
        }
    }

    for (int i = 0; i < global_objects.relationship_count; i++) {
        Relationship *r = global_objects.relationships[i];
        if (!r)
            continue;
        if (!r->e1 || !r->e2)
            continue;

        SNAP_WRITE("create relationship \"%s\" \"%s\" \"%s\";\n", r->name, r->e1->name, r->e2->name);

        // Properties on the relationship
        for (int j = 0; j < r->num_properties; j++) {
            Property *p = r->properties[j];
            if (!p)
                continue;
            SNAP_WRITE("add property \"%s\" \"%s\" %s%s;\n", r->name, p->name, p->type, key_type_str(p->keytype));
        }

        // Cardinality: reconstruct the "min1,max1,min2,max2" raw string
        if (r->cards[0] && r->cards[1]) {
            // cards[n]->value is stored as "x,y\0" (CARDINALITY_LEN = 4)
            // We need to combine them into one "x,y,x,y" raw string
            char raw[RAW_CARDINALITY_LEN]; // 9 bytes: "x,y,x,y\0"
            snprintf(raw, RAW_CARDINALITY_LEN, "%c,%c,%c,%c", r->cards[0]->value[0], r->cards[0]->value[2],
                     r->cards[1]->value[0], r->cards[1]->value[2]);
            SNAP_WRITE("add card \"%s\" \"%s\";\n", r->name, raw);
        }
    }

#undef SNAP_WRITE

    if (pos < buf_size) {
        buf[pos] = '\0';
    } else {
        buf[buf_size - 1] = '\0';
    }

    return pos;
}
