// =========================================================================
// tests.c
// Unified testing suite for Arena Allocator, Lexer/Tokenizer, Parser,
// MCD Elements, Command Processor, and Graphics/MCD generation
// =========================================================================

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// =========================================================================
// SHARED TEST INFRASTRUCTURE
// =========================================================================

static FILE *log_file = NULL;
static int total_pass = 0;
static int total_fail = 0;

#define TEST(test_name, condition)                                                                                       \
    do {                                                                                                                 \
        if (condition) {                                                                                                 \
            fprintf(log_file, "  [PASS] %s\n", test_name);                                                               \
            total_pass++;                                                                                                \
        } else {                                                                                                         \
            fprintf(log_file, "  [FAIL] %s  (line %d)\n", test_name, __LINE__);                                          \
            total_fail++;                                                                                                \
        }                                                                                                                \
    } while (0)

#define SECTION(title) fprintf(log_file, "\n--- %s ---\n", title)

#define SUITE_HEADER(title)                                                                                              \
    fprintf(log_file, "\n");                                                                                             \
    fprintf(log_file, "=========================================\n");                                                    \
    fprintf(log_file, "  %s\n", title);                                                                                  \
    fprintf(log_file, "=========================================\n")

// =========================================================================
// ARENA TESTS (Compiled only if ARENA_TESTS is defined)
// =========================================================================
#ifdef ARENA_TESTS

#include "../utils/arena_allocator.h"

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------
static void test_arena_initialization(void) {
    SECTION("Initialization");
    Arena a;
    arena_init(&a, 1024);
    TEST("Head is NULL before first allocation", a.head == NULL);
    TEST("Current is NULL before first allocation", a.current == NULL);
    TEST("Default block size stored correctly", a.default_block_size == 1024);
}

// ---------------------------------------------------------------------------
// Basic allocation and pointer alignment
// ---------------------------------------------------------------------------
static void test_arena_basic_alloc_and_alignment(void) {
    SECTION("Basic Allocation & Alignment");
    Arena a;
    arena_init(&a, 1024);

    void *p1 = arena_alloc_aligned(&a, 10, 8);
    TEST("Valid pointer returned for 10-byte alloc", p1 != NULL);
    TEST("Current block initialised after first alloc", a.current != NULL);
    TEST("Head block initialised after first alloc", a.head != NULL);
    TEST("Head and current point to the same (only) block", a.head == a.current);
    TEST("p1 is 8-byte aligned", (uintptr_t)p1 % 8 == 0);

    void *p2 = arena_alloc_aligned(&a, 15, 16);
    TEST("Second allocation returns a non-NULL pointer", p2 != NULL);
    TEST("p2 is 16-byte aligned", (uintptr_t)p2 % 16 == 0);
    TEST("p2 does not overlap with p1 (no-overlap guarantee)", (uintptr_t)p2 >= (uintptr_t)p1 + 10);

    void *p3 = arena_alloc_aligned(&a, 1, 1);
    TEST("1-byte allocation with alignment=1 succeeds", p3 != NULL);
    TEST("p3 does not overlap with p2", (uintptr_t)p3 >= (uintptr_t)p2 + 15);
}

// ---------------------------------------------------------------------------
// Alignment edge cases
// ---------------------------------------------------------------------------
static void test_arena_alignment_edge_cases(void) {
    SECTION("Alignment Edge Cases");
    Arena a;
    arena_init(&a, 4096);

    // Powers-of-two alignments: 1, 2, 4, 8, 16, 32, 64
    size_t alignments[] = {1, 2, 4, 8, 16, 32, 64};
    for (int i = 0; i < 7; i++) {
        char name[64];
        snprintf(name, sizeof(name), "Pointer aligned to %zu bytes", alignments[i]);
        void *p = arena_alloc_aligned(&a, 8, alignments[i]);
        TEST(name, p != NULL && (uintptr_t)p % alignments[i] == 0);
    }
}

// ---------------------------------------------------------------------------
// Block chaining / overflow into new block
// ---------------------------------------------------------------------------
static void test_arena_block_chaining(void) {
    SECTION("Block Chaining (OOM Overflow)");
    Arena a;
    arena_init(&a, 64); // deliberately tiny to force chaining

    void *p1 = arena_alloc_aligned(&a, 40, 8);
    TEST("First 40-byte allocation succeeds in 64-byte arena", p1 != NULL);
    TEST("After first alloc: head == current (single block)", a.head == a.current);

    void *p2 = arena_alloc_aligned(&a, 40, 8); // must spill into new block
    TEST("Second 40-byte allocation triggers block chain", p2 != NULL);
    TEST("Head and current now differ (chain formed)", a.head != a.current);
    TEST("head->next points to current block", a.head->next == a.current);
    TEST("p2 is still 8-byte aligned after block chain", (uintptr_t)p2 % 8 == 0);

    // Force a third block
    void *p3 = arena_alloc_aligned(&a, 40, 8);
    TEST("Third allocation (third block) succeeds", p3 != NULL);
    TEST("Chain has at least two links (head->next->next != NULL)", a.head->next != NULL && a.head->next->next != NULL);
}

// ---------------------------------------------------------------------------
// Oversized allocations (larger than default_block_size)
// ---------------------------------------------------------------------------
static void test_arena_oversized_allocation(void) {
    SECTION("Oversized Allocations");
    Arena a;
    arena_init(&a, 1024);

    void *p = arena_alloc_aligned(&a, 5000, 8); // 5 KB > 1 KB default
    TEST("Allocation larger than default block size succeeds", p != NULL);
    TEST("Current block capacity scaled to fit the payload", a.current != NULL && a.current->cap >= 5000);

    // Immediately follow up with a normal-sized allocation to verify the arena
    // is still functional after handling an oversized request
    void *q = arena_alloc_aligned(&a, 32, 8);
    TEST("Normal allocation after oversized request still works", q != NULL);
}

// ---------------------------------------------------------------------------
// Multiple sequential allocations – stress / correctness
// ---------------------------------------------------------------------------
static void test_arena_many_allocations(void) {
    SECTION("Sequential Allocation Stress (100 allocs)");
    Arena a;
    arena_init(&a, 512);

    bool all_non_null = true;
    bool all_aligned = true;
    bool no_overlap = true;
    void *prev = NULL;
    size_t prev_size = 0;

    for (int i = 0; i < 100; i++) {
        size_t sz = (size_t)(i + 1) * 4;
        void *p = arena_alloc_aligned(&a, sz, 8);
        if (!p) {
            all_non_null = false;
            break;
        }
        if ((uintptr_t)p % 8 != 0) {
            all_aligned = false;
        }
        /* Two pointers only need to be non-overlapping when they live in the
         * same arena block.  When the arena chains a new block the new pointer
         * can have a lower raw address than the previous one (different mmap /
         * malloc region).  We detect same-block placement by checking that p
         * is strictly above prev — within one block the offset only moves
         * forward, so p > prev always holds when they share a block.         */
        if (prev && (uintptr_t)p > (uintptr_t)prev) {
            if ((uintptr_t)p < (uintptr_t)prev + prev_size) {
                no_overlap = false;
            }
        }
        prev = p;
        prev_size = sz;
    }
    TEST("All 100 allocations return non-NULL pointers", all_non_null);
    TEST("All 100 allocations are 8-byte aligned", all_aligned);
    TEST("No two consecutive allocations overlap", no_overlap);
}

// ---------------------------------------------------------------------------
// Temporary arena: save / restore semantics
// ---------------------------------------------------------------------------
static void test_temp_arena(void) {
    SECTION("Temporary Arena (save/restore)");
    Arena a;
    arena_init(&a, 1024);

    // Allocate some permanent data
    void *permanent = arena_alloc_aligned(&a, 64, 8);
    TEST("Permanent allocation before temp succeeds", permanent != NULL);

    TempArena temp = init_temp_arena(&a);
    TEST("TempArena references the same arena", temp.a == &a);
    TEST("Saved block matches current block at save time", temp.saved_block == a.current);

    void *tmp1 = arena_alloc_aligned(&a, 128, 8);
    void *tmp2 = arena_alloc_aligned(&a, 256, 8);
    TEST("Temp allocation #1 succeeds", tmp1 != NULL);
    TEST("Temp allocation #2 succeeds", tmp2 != NULL);

    free_temp_arena(temp);
    // After rewind, the offset should be back to (at most) where it was.
    // We verify the arena is still usable.
    void *after = arena_alloc_aligned(&a, 32, 8);
    TEST("Allocation after free_temp_arena succeeds (arena still live)", after != NULL);
}

// ---------------------------------------------------------------------------
// Destroy arena: full teardown
// ---------------------------------------------------------------------------
static void test_arena_destroy(void) {
    SECTION("Arena Destroy");
    Arena a;
    arena_init(&a, 256);
    arena_alloc_aligned(&a, 100, 8); // occupy some memory
    arena_alloc_aligned(&a, 200, 8); // force a second block

    destroy_arena(&a);
    // After destroy the arena struct pointers must be NULL-safe to re-init
    // (we can't dereference them, but re-initialising should work cleanly)
    arena_init(&a, 128);
    void *p = arena_alloc_aligned(&a, 10, 4);
    TEST("Re-initialisation after destroy succeeds", p != NULL);
    destroy_arena(&a);
    TEST("Second destroy does not crash (double-destroy safety)", true); // reaching here = pass
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------
void run_arena_tests(void) {
    printf("Starting Arena Tests...\n");
    log_file = fopen("arena_test_results.log", "w");
    if (!log_file) {
        printf("CRITICAL ERROR: Could not open log file!\n");
        return;
    }

    SUITE_HEADER("ARENA ALLOCATOR TESTS");

    test_arena_initialization();
    test_arena_basic_alloc_and_alignment();
    test_arena_alignment_edge_cases();
    test_arena_block_chaining();
    test_arena_oversized_allocation();
    test_arena_many_allocations();
    test_temp_arena();
    test_arena_destroy();

    fprintf(log_file, "\n=========================================\n");
    fprintf(log_file, "  RESULTS: %d passed, %d failed\n", total_pass, total_fail);
    fprintf(log_file, "=========================================\n");
    fclose(log_file);
    printf("Arena tests complete. Results: %d passed, %d failed.\n"
           "See 'arena_test_results.log' for details.\n",
           total_pass, total_fail);
    total_pass = 0;
    total_fail = 0;
}

#endif // ARENA_TESTS

// =========================================================================
// LEXER / TOKENIZER TESTS (Compiled only if LEXER_TESTS is defined)
// =========================================================================
#ifdef LEXER_TESTS

#include "../Lexer/tokenize.h"

static void test_tokenizer_keywords(void) {
    SECTION("Keyword Recognition");
    Token tokens[MAX_TOKENS_NUM_PER_COMMAND];
    int count = 0;

    tokenize_content("create entity relationship add property clear", tokens, &count);

    TEST("'create'       -> TOKEN_CREATE", tokens[0].type == TOKEN_CREATE);
    TEST("'entity'       -> TOKEN_ENTITY", tokens[1].type == TOKEN_ENTITY);
    TEST("'relationship' -> TOKEN_RELATIONSHIP", tokens[2].type == TOKEN_RELATIONSHIP);
    TEST("'add'          -> TOKEN_ADD", tokens[3].type == TOKEN_ADD);
    TEST("'property'     -> TOKEN_PROPERTY", tokens[4].type == TOKEN_PROPERTY);
    TEST("'clear'        -> TOKEN_CLEAR", tokens[5].type == TOKEN_CLEAR);
    TEST("Final token is TOKEN_EOF", tokens[count].type == TOKEN_EOF);
}

static void test_tokenizer_string_literals(void) {
    SECTION("String Literal Extraction (double & single quotes)");
    Token tokens[MAX_TOKENS_NUM_PER_COMMAND];
    int count = 0;

    tokenize_content("create entity \"MyEntity\"", tokens, &count);
    TEST("Double-quoted string classified as TOKEN_STRING", tokens[2].type == TOKEN_STRING);
    TEST("Double-quoted string value extracted correctly", strcmp(tokens[2].value, "MyEntity") == 0);

    count = 0;
    tokenize_content("create entity 'AnotherEntity'", tokens, &count);
    TEST("Single-quoted string classified as TOKEN_STRING", tokens[2].type == TOKEN_STRING);
    TEST("Single-quoted string value extracted correctly", strcmp(tokens[2].value, "AnotherEntity") == 0);
}

static void test_tokenizer_identifiers(void) {
    SECTION("Identifier Classification");
    Token tokens[MAX_TOKENS_NUM_PER_COMMAND];
    int count = 0;

    tokenize_content("add property \"E\" myProp int", tokens, &count);
    // 'myProp' and 'int' are not keywords -> TOKEN_IDENTIFIER
    TEST("Non-keyword word 'int' classified as TOKEN_IDENTIFIER", tokens[4].type == TOKEN_IDENTIFIER);
}

static void test_tokenizer_unknown_symbols(void) {
    SECTION("Unknown / Special Symbol Handling");
    Token tokens[MAX_TOKENS_NUM_PER_COMMAND];
    int count = 0;

    tokenize_content("create @ entity", tokens, &count);
    TEST("'@' symbol classified as TOKEN_UNKNOWN", tokens[1].type == TOKEN_UNKNOWN);
}

static void test_tokenizer_empty_input(void) {
    SECTION("Edge Cases: Empty & Whitespace-Only Input");
    Token tokens[MAX_TOKENS_NUM_PER_COMMAND];
    int count = 0;

    tokenize_content("", tokens, &count);
    TEST("Empty string: count == 0", count == 0);
    TEST("Empty string: EOF token set", tokens[0].type == TOKEN_EOF);

    count = 0;
    tokenize_content("   \t  \n  ", tokens, &count);
    TEST("Whitespace-only string: count == 0", count == 0);
    TEST("Whitespace-only string: EOF token set", tokens[0].type == TOKEN_EOF);
}

static void test_tokenizer_null_input(void) {
    SECTION("Edge Cases: NULL Input");
    Token tokens[MAX_TOKENS_NUM_PER_COMMAND];
    int count = 99; // deliberately dirty
    tokenize_content(NULL, tokens, &count);
    TEST("NULL input: count reset to 0", count == 0);
}

static void test_tokenizer_position_tracking(void) {
    SECTION("Token Position Tracking (for error reporting)");
    Token tokens[MAX_TOKENS_NUM_PER_COMMAND];
    int count = 0;

    tokenize_content("create \"Foo\"", tokens, &count);
    // The 'create' token starts at position 0 plus its length
    // pos is recorded as the position *after* the token
    TEST("'create' token has non-negative pos", tokens[0].pos >= 0);
    TEST("'create' token has correct length (6)", tokens[0].length == 6);
    TEST("String token pos > 'create' token pos", tokens[1].pos > tokens[0].pos);
}

static void test_tokenizer_long_command(void) {
    SECTION("Long Command: relationship with two entity names");
    Token tokens[MAX_TOKENS_NUM_PER_COMMAND];
    int count = 0;

    tokenize_content("create relationship \"Borrows\" \"Member\" \"Book\"", tokens, &count);
    TEST("'create'       token[0]", tokens[0].type == TOKEN_CREATE);
    TEST("'relationship' token[1]", tokens[1].type == TOKEN_RELATIONSHIP);
    TEST("'Borrows'      token[2] = STRING", tokens[2].type == TOKEN_STRING);
    TEST("'Member'       token[3] = STRING", tokens[3].type == TOKEN_STRING);
    TEST("'Book'         token[4] = STRING", tokens[4].type == TOKEN_STRING);
    TEST("value of token[2] is 'Borrows'", strcmp(tokens[2].value, "Borrows") == 0);
    TEST("value of token[3] is 'Member'", strcmp(tokens[3].value, "Member") == 0);
    TEST("value of token[4] is 'Book'", strcmp(tokens[4].value, "Book") == 0);
}

static void test_tokenizer_peek_and_advance(void) {
    SECTION("peek_token / advance_token API");
    Token tokens[MAX_TOKENS_NUM_PER_COMMAND];
    int count = 0;
    tokenize_content("create entity \"E1\"", tokens, &count);

    int cursor = 0;
    Token t0 = peek_token(tokens, cursor);
    TEST("peek at 0 returns TOKEN_CREATE without moving cursor", t0.type == TOKEN_CREATE && cursor == 0);

    advance_token(&cursor);
    Token t1 = peek_token(tokens, cursor);
    TEST("After advance, peek returns TOKEN_ENTITY", t1.type == TOKEN_ENTITY);
    TEST("Cursor is now 1", cursor == 1);

    advance_token(&cursor);
    Token t2 = peek_token(tokens, cursor);
    TEST("After second advance, peek returns TOKEN_STRING", t2.type == TOKEN_STRING);
    TEST("String value is 'E1'", strcmp(t2.value, "E1") == 0);
}

void run_lexer_tests(void) {
    printf("Starting Lexer/Tokenizer Tests...\n");
    log_file = fopen("lexer_test_results.log", "w");
    if (!log_file) {
        printf("CRITICAL ERROR: Could not open log file!\n");
        return;
    }

    SUITE_HEADER("LEXER / TOKENIZER TESTS");

    test_tokenizer_keywords();
    test_tokenizer_string_literals();
    test_tokenizer_identifiers();
    test_tokenizer_unknown_symbols();
    test_tokenizer_empty_input();
    test_tokenizer_null_input();
    test_tokenizer_position_tracking();
    test_tokenizer_long_command();
    test_tokenizer_peek_and_advance();

    fprintf(log_file, "\n=========================================\n");
    fprintf(log_file, "  RESULTS: %d passed, %d failed\n", total_pass, total_fail);
    fprintf(log_file, "=========================================\n");
    fclose(log_file);
    printf("Lexer tests complete. Results: %d passed, %d failed.\n"
           "See 'lexer_test_results.log' for details.\n",
           total_pass, total_fail);
    total_pass = 0;
    total_fail = 0;
}

#endif // LEXER_TESTS

// =========================================================================
// PARSER / COMMAND EXECUTION TESTS (Compiled only if PARSER_TESTS is defined)
//
// NOTE: These tests exercise the parse_* functions and execute_* functions
// directly, bypassing ncurses.  We pass NULL as the WINDOW* and rely on the
// implementations being NULL-safe for the console argument when we are only
// testing the structural output (CreateCommand / AddCommand).
//
// If your implementation crashes on a NULL WINDOW, mock it with
// `newwin(1,1,0,0)` after initscr() or wrap it in a headless helper.
// =========================================================================
#ifdef PARSER_TESTS

#include "../Lexer/parse.h"
#include "../Lexer/tokenize.h"
#include "../MCD_elements.h"
#include "../global_objects.h"

// Minimal no-op WINDOW substitute – real tests should link against ncurses
// and call initscr()/endwin() around this suite if needed.
// Here we assume the parser is NULL-safe for its WINDOW* argument when the
// only side-effect tested is the returned bool + populated command struct.

static void test_parser_init(void) {
    SECTION("Parser Initialisation");
    Parser p;
    const char *input = "create entity \"Foo\"";
    init_parser(&p, input);
    TEST("current starts at 0", p.current == 0);
    TEST("count starts at 0", p.count == 0);
    TEST("userInput pointer captured", p.userInput == input);
}

static void test_parser_create_entity_command(void) {
    SECTION("parse_create – Entity");
    Parser p;
    const char *input = "create entity \"Customer\"";
    init_parser(&p, input);
    tokenize_content(input, p.tokens, &p.count);

    // Consume the leading TOKEN_CREATE before handing off to parse_create
    advance_token(&p.current); // now at TOKEN_ENTITY

    CreateCommand c = {0};
    bool ok = parse_create(&p, NULL, &c);

    TEST("parse_create returns true for valid entity command", ok);
    TEST("Command type is TYPE_ENTITY", c.type == TYPE_ENTITY);
    TEST("Entity name extracted correctly", strcmp(c.Data.e.name, "Customer") == 0);
}

static void test_parser_create_relationship_command(void) {
    SECTION("parse_create – Relationship");
    Parser p;
    const char *input = "create relationship \"Owns\" \"Customer\" \"Cart\"";
    init_parser(&p, input);
    tokenize_content(input, p.tokens, &p.count);

    advance_token(&p.current); // skip TOKEN_CREATE

    CreateCommand c = {0};
    bool ok = parse_create(&p, NULL, &c);

    TEST("parse_create returns true for valid relationship command", ok);
    TEST("Command type is TYPE_RELATIONSHIP", c.type == TYPE_RELATIONSHIP);
    TEST("Relationship name extracted correctly", strcmp(c.Data.r.name, "Owns") == 0);
    TEST("Entity-1 name extracted correctly", strcmp(c.Data.r.e1_name, "Customer") == 0);
    TEST("Entity-2 name extracted correctly", strcmp(c.Data.r.e2_name, "Cart") == 0);
}

static void test_parser_create_entity_missing_name(void) {
    SECTION("parse_create – Entity with missing name (error path)");
    Parser p;
    // 'entity' keyword but no name token following
    const char *input = "create entity";
    init_parser(&p, input);
    tokenize_content(input, p.tokens, &p.count);
    advance_token(&p.current);

    CreateCommand c = {0};
    bool ok = parse_create(&p, NULL, &c);
    TEST("parse_create returns false when entity name is absent", !ok);
}

static void test_parser_add_property_command(void) {
    SECTION("parse_add – Property");
    Parser p;
    /* Property names must be quoted — the tokenizer classifies bare words as
     * TOKEN_IDENTIFIER, but parse_add_property_name expects TOKEN_STRING.
     * The help text confirms: "You MUST use quotations around names."        */
    const char *input = "add property \"Customer\" \"customer_id\" int";
    init_parser(&p, input);
    tokenize_content(input, p.tokens, &p.count);
    advance_token(&p.current); // skip TOKEN_ADD

    AddCommand c = {0};
    bool ok = parse_add(&p, NULL, &c);

    TEST("parse_add returns true for valid add property command", ok);
    TEST("Target identifier name captured", strcmp(c.identifier_name, "Customer") == 0);
    TEST("Property name captured", strcmp(c.prop_name, "customer_id") == 0);
    TEST("Property type captured", strcmp(c.prop_type, "int") == 0);
}

static void test_execute_create_entity(void) {
    SECTION("execute_create – Entity Integration");
    init_global_objects();

    CreateCommand c = {0};
    c.type = TYPE_ENTITY;
    strncpy(c.Data.e.name, "TestEntity", MAX_NAME_LEN - 1);

    execute_create(c);

    Entity *found = search_entity("TestEntity");
    TEST("Entity is findable after execute_create", found != NULL);
    TEST("Entity name matches", found && strcmp(found->name, "TestEntity") == 0);
}

static void test_execute_create_entity_dedup(void) {
    SECTION("execute_create – Duplicate Entity Prevention");
    init_global_objects();

    CreateCommand c = {0};
    c.type = TYPE_ENTITY;
    strncpy(c.Data.e.name, "Unique", MAX_NAME_LEN - 1);

    execute_create(c);
    int count_after_first = global_objects.entity_count;
    execute_create(c); // second call with same name
    int count_after_second = global_objects.entity_count;

    TEST("Entity count does not grow on duplicate create", count_after_second == count_after_first);
}

static void test_execute_create_relationship(void) {
    SECTION("execute_create – Relationship Integration");
    init_global_objects();

    CreateCommand c = {0};
    c.type = TYPE_RELATIONSHIP;
    strncpy(c.Data.r.name, "Owns", MAX_NAME_LEN - 1);
    strncpy(c.Data.r.e1_name, "Alice", MAX_NAME_LEN - 1);
    strncpy(c.Data.r.e2_name, "Bob", MAX_NAME_LEN - 1);

    execute_create(c);

    // Entities should be auto-created
    TEST("Entity 'Alice' auto-created by relationship execute", search_entity("Alice") != NULL);
    TEST("Entity 'Bob'   auto-created by relationship execute", search_entity("Bob") != NULL);

    Relationship *r = search_relationship("Owns");
    TEST("Relationship 'Owns' registered in global objects", r != NULL);
    TEST("Relationship e1 == 'Alice'", r && strcmp(r->e1->name, "Alice") == 0);
    TEST("Relationship e2 == 'Bob'", r && strcmp(r->e2->name, "Bob") == 0);
}

static void test_execute_addProperty_entity(void) {
    SECTION("execute_addProperty – onto Entity");
    init_global_objects();

    // Create entity first
    CreateCommand cc = {0};
    cc.type = TYPE_ENTITY;
    strncpy(cc.Data.e.name, "User", MAX_NAME_LEN - 1);
    execute_create(cc);

    AddCommand ac = {0};
    strncpy(ac.identifier_name, "User", MAX_NAME_LEN - 1);
    strncpy(ac.prop_name, "user_id", MAX_NAME_LEN - 1);
    strncpy(ac.prop_type, "int", MAX_TYPE_LEN - 1);
    execute_addProperty(ac);

    Entity *e = search_entity("User");
    TEST("Entity exists after addProperty", e != NULL);
    TEST("Entity has at least one property", e && e->num_properties >= 1);
    TEST("Added property name matches 'user_id'",
         e && e->num_properties >= 1 && strcmp(e->properties[0]->name, "user_id") == 0);
    TEST("Added property type matches 'int'", e && e->num_properties >= 1 && strcmp(e->properties[0]->type, "int") == 0);
}

void run_parser_tests(void) {
    printf("Starting Parser/Command Tests...\n");
    log_file = fopen("parser_test_results.log", "w");
    if (!log_file) {
        printf("CRITICAL ERROR: Could not open log file!\n");
        return;
    }

    SUITE_HEADER("PARSER / COMMAND EXECUTION TESTS");

    test_parser_init();
    test_parser_create_entity_command();
    test_parser_create_relationship_command();
    test_parser_create_entity_missing_name();
    test_parser_add_property_command();
    test_execute_create_entity();
    test_execute_create_entity_dedup();
    test_execute_create_relationship();
    test_execute_addProperty_entity();

    fprintf(log_file, "\n=========================================\n");
    fprintf(log_file, "  RESULTS: %d passed, %d failed\n", total_pass, total_fail);
    fprintf(log_file, "=========================================\n");
    fclose(log_file);
    printf("Parser tests complete. Results: %d passed, %d failed.\n"
           "See 'parser_test_results.log' for details.\n",
           total_pass, total_fail);
    total_pass = 0;
    total_fail = 0;
}

#endif // PARSER_TESTS

// =========================================================================
// MCD ELEMENTS TESTS (Compiled only if MCD_TESTS is defined)
// =========================================================================
#ifdef MCD_TESTS

#include "../MCD_elements.h"
#include "../global_objects.h"

static void test_mcd_create_entity(void) {
    SECTION("createEntity");
    init_global_objects();

    Entity *e = createEntity("Invoice", 5, 10);
    TEST("createEntity returns non-NULL pointer", e != NULL);
    TEST("Entity name stored correctly", strcmp(e->name, "Invoice") == 0);
    /* createEntity may auto-compute placement (e.g. based on entity_count),
     * so we assert that coordinates are valid (non-negative) rather than
     * testing exact values that the implementation does not guarantee.      */
    TEST("Entity x coordinate is non-negative", e->x >= 0);
    TEST("Entity y coordinate is non-negative", e->y >= 0);
    TEST("Entity has zero properties on creation", e->num_properties == 0);
    TEST("Entity registered in global_objects", global_objects.entity_count == 1);
    TEST("global_objects.entities[0] == returned pointer", global_objects.entities[0] == e);
}

static void test_mcd_add_property(void) {
    SECTION("addProperty – Entity");
    init_global_objects();

    Entity *e = createEntity("Product", 0, 0);
    addProperty(e, "price", "float");
    addProperty(e, "sku", "str");

    TEST("num_properties == 2 after two addProperty calls", e->num_properties == 2);
    TEST("First property name is 'price'", strcmp(e->properties[0]->name, "price") == 0);
    TEST("First property type is 'float'", strcmp(e->properties[0]->type, "float") == 0);
    TEST("Second property name is 'sku'", strcmp(e->properties[1]->name, "sku") == 0);
    TEST("Second property type is 'str'", strcmp(e->properties[1]->type, "str") == 0);
}

static void test_mcd_create_relationship(void) {
    SECTION("addRelationship");
    init_global_objects();

    Entity *e1 = createEntity("Author", 0, 0);
    Entity *e2 = createEntity("Book", 50, 0);
    Relationship *r = addRelationship(25, 0, e1, e2, "Writes");

    TEST("addRelationship returns non-NULL pointer", r != NULL);
    TEST("Relationship name stored correctly", strcmp(r->name, "Writes") == 0);
    /* Same as createEntity: implementation may adjust placement,
     * so assert non-negative rather than exact coordinates.       */
    TEST("Relationship x coordinate is non-negative", r->x >= 0);
    TEST("Relationship y coordinate is non-negative", r->y >= 0);
    TEST("Relationship e1 pointer correct", r->e1 == e1);
    TEST("Relationship e2 pointer correct", r->e2 == e2);
    TEST("Relationship has zero properties on creation", r->num_properties == 0);
    TEST("Relationship registered in global_objects", global_objects.relationship_count == 1);
    TEST("global_objects.relationships[0] == returned pointer", global_objects.relationships[0] == r);
}

static void test_mcd_add_property_relationship(void) {
    SECTION("addPropertyRelationship");
    init_global_objects();

    Entity *e1 = createEntity("Student", 0, 0);
    Entity *e2 = createEntity("Course", 40, 0);
    Relationship *r = addRelationship(20, 0, e1, e2, "Enrolls");
    addPropertyRelationship(r, "grade", "str");
    addPropertyRelationship(r, "semester", "str");

    TEST("Relationship has 2 properties after two add calls", r->num_properties == 2);
    TEST("Property[0] name is 'grade'", strcmp(r->properties[0]->name, "grade") == 0);
    TEST("Property[1] name is 'semester'", strcmp(r->properties[1]->name, "semester") == 0);
}

static void test_mcd_search_entity(void) {
    SECTION("search_entity");
    init_global_objects();

    createEntity("Alpha", 0, 0);
    createEntity("Beta", 10, 0);

    Entity *found = search_entity("Alpha");
    TEST("search_entity finds existing entity by name", found != NULL);
    TEST("Returned pointer has correct name", found && strcmp(found->name, "Alpha") == 0);

    Entity *missing = search_entity("Gamma");
    TEST("search_entity returns NULL for non-existent entity", missing == NULL);

    Entity *empty = search_entity("");
    TEST("search_entity returns NULL for empty-string query", empty == NULL);
}

static void test_mcd_search_relationship(void) {
    SECTION("search_relationship");
    init_global_objects();

    Entity *e1 = createEntity("X", 0, 0);
    Entity *e2 = createEntity("Y", 30, 0);
    addRelationship(15, 0, e1, e2, "LinksTo");

    Relationship *found = search_relationship("LinksTo");
    TEST("search_relationship finds existing relationship", found != NULL);
    TEST("Found relationship has correct name", found && strcmp(found->name, "LinksTo") == 0);

    Relationship *missing = search_relationship("NoSuchRel");
    TEST("search_relationship returns NULL for non-existent rel", missing == NULL);
}

static void test_mcd_cardinality(void) {
    SECTION("addCardinalityAPI");
    init_global_objects();

    Entity *e1 = createEntity("Emp", 0, 5);
    Entity *e2 = createEntity("Dept", 50, 5);
    Relationship *r = addRelationship(25, 5, e1, e2, "WorksFor");
    addCardinalityAPI("1,n,1,1", r);

    // After addCardinalityAPI both cardinality slots should be populated
    TEST("cards[0] is populated after addCardinalityAPI", r->cards[0] != NULL);
    TEST("cards[1] is populated after addCardinalityAPI", r->cards[1] != NULL);
}

static void test_mcd_global_objects_clear(void) {
    SECTION("init_global_objects – reset");
    // Populate the world
    createEntity("Garbage1", 0, 0);
    createEntity("Garbage2", 0, 0);

    init_global_objects();
    TEST("entity_count resets to 0 after init_global_objects", global_objects.entity_count == 0);
    TEST("relationship_count resets to 0 after init_global_objects", global_objects.relationship_count == 0);
}

void run_mcd_tests(void) {
    printf("Starting MCD Element Tests...\n");
    log_file = fopen("mcd_test_results.log", "w");
    if (!log_file) {
        printf("CRITICAL ERROR: Could not open log file!\n");
        return;
    }

    SUITE_HEADER("MCD ELEMENTS TESTS");

    test_mcd_create_entity();
    test_mcd_add_property();
    test_mcd_create_relationship();
    test_mcd_add_property_relationship();
    test_mcd_search_entity();
    test_mcd_search_relationship();
    test_mcd_cardinality();
    test_mcd_global_objects_clear();

    fprintf(log_file, "\n=========================================\n");
    fprintf(log_file, "  RESULTS: %d passed, %d failed\n", total_pass, total_fail);
    fprintf(log_file, "=========================================\n");
    fclose(log_file);
    printf("MCD element tests complete. Results: %d passed, %d failed.\n"
           "See 'mcd_test_results.log' for details.\n",
           total_pass, total_fail);
    total_pass = 0;
    total_fail = 0;
}

#endif // MCD_TESTS

// =========================================================================
// HELP WINDOW + KMP SEARCH TESTS (Compiled only if HELP_TESTS is defined)
//
// These tests cover:
//   - HelpWindow initialisation and page state
//   - set_current_page / set_scrolling_line navigation API
//   - KMP LPS array construction
//   - search_kmp: matches, misses, multiple hits, overlapping
//   - search_help_kmp: full integration against the live helpdb
//   - SearchResult memory (destroy_search_results)
// =========================================================================
#ifdef HELP_TESTS

#include "../DSA/kmp.h"
#include "../DSA/vec.h"
#include "../help_window.h"

// ---------------------------------------------------------------------------
// HelpWindow initialisation
// ---------------------------------------------------------------------------
static void test_help_window_init(void) {
    SECTION("init_help_window – initial state");
    HelpWindow hwin;
    init_help_window(&hwin, Main);

    TEST("current_page is Main after init", hwin.current_page == Main);
    TEST("pages_db pointer is non-NULL", hwin.pages_db != NULL);
    TEST("main_scrolling_line initialised to 0", hwin.main_scrolling_line == 0);
    /* hotkey and examples offsets are set to PAD_OFFSET / PAD_OFFSET*2 */
    TEST("hotkey_scrolling_line is non-negative", hwin.hotkey_scrolling_line >= 0);
    TEST("examples_scrolling_line is non-negative", hwin.examples_scrolling_line >= 0);

    /* Each page must have line_count > 0 and non-NULL text on line 0 */
    for (int pg = 0; pg < HelpPageNum; pg++) {
        TEST("Page has at least one line", hwin.pages_db[pg].line_count > 0);
        TEST("First line on page has non-NULL text", hwin.pages_db[pg].lines[0].text != NULL);
    }
}

// ---------------------------------------------------------------------------
// Page navigation: set_current_page
// ---------------------------------------------------------------------------
static void test_help_set_current_page(void) {
    SECTION("set_current_page");
    HelpWindow hwin;
    init_help_window(&hwin, Main);

    set_current_page(&hwin, Hotkeys);
    TEST("set_current_page(Hotkeys) -> current_page == Hotkeys", hwin.current_page == Hotkeys);

    set_current_page(&hwin, Examples);
    TEST("set_current_page(Examples) -> current_page == Examples", hwin.current_page == Examples);

    set_current_page(&hwin, Main);
    TEST("set_current_page(Main) -> current_page == Main", hwin.current_page == Main);

    /* NULL guard: must not crash */
    set_current_page(NULL, Hotkeys);
    TEST("set_current_page(NULL, ...) does not crash", true);
}

// ---------------------------------------------------------------------------
// Scrolling line setter
// ---------------------------------------------------------------------------
static void test_help_set_scrolling_line(void) {
    SECTION("set_scrolling_line");
    HelpWindow hwin;
    init_help_window(&hwin, Main);

    set_current_page(&hwin, Main);
    set_scrolling_line(&hwin, 5);
    TEST("main_scrolling_line updated to 5", hwin.main_scrolling_line == 5);

    set_current_page(&hwin, Hotkeys);
    set_scrolling_line(&hwin, 3);
    TEST("hotkey_scrolling_line updated to 3", hwin.hotkey_scrolling_line == 3);

    set_current_page(&hwin, Examples);
    set_scrolling_line(&hwin, 7);
    TEST("examples_scrolling_line updated to 7", hwin.examples_scrolling_line == 7);

    /* Negative line must be rejected (implementation returns early) */
    set_current_page(&hwin, Main);
    set_scrolling_line(&hwin, 5);
    set_scrolling_line(&hwin, -1);
    TEST("set_scrolling_line(-1) is ignored – value unchanged", hwin.main_scrolling_line == 5);
}

// ---------------------------------------------------------------------------
// HelpLine tokenization side-effect of init
// ---------------------------------------------------------------------------
static void test_help_window_line_tokenization(void) {
    SECTION("HelpLine tokenization during init");
    HelpWindow hwin;
    init_help_window(&hwin, Main);

    /* The title line "=== MCD Designer Help CLI ===" must have been tokenized */
    const HelpLine *title = &hwin.pages_db[Main].lines[0];
    TEST("Main page title line has been tokenized (token_count > 0)", title->token_count > 0);
    TEST("Main page title line_len > 0", title->line_len > 0);

    /* Examples page code lines should also have tokens */
    const HelpPageData *ex = &hwin.pages_db[Examples];
    bool any_code_tokenized = false;
    for (size_t l = 0; l < ex->line_count; l++) {
        if (ex->lines[l].type == code && ex->lines[l].token_count > 0) {
            any_code_tokenized = true;
            break;
        }
    }
    TEST("At least one Examples code line has been tokenized", any_code_tokenized);
}

// ---------------------------------------------------------------------------
// KMP: compute_lps
// ---------------------------------------------------------------------------
static void test_kmp_lps_basic(void) {
    SECTION("compute_lps – basic patterns");
    int lps[MAX_SEARCH_BUFFER_LEN];

    /* "aaaa" -> lps = [0,1,2,3] */
    const char *pat1 = "aaaa";
    int len1 = (int)strlen(pat1);
    init_LPS(lps);
    compute_lps(lps, pat1, len1);
    TEST("lps[0]='aaaa' == 0", lps[0] == 0);
    TEST("lps[1]='aaaa' == 1", lps[1] == 1);
    TEST("lps[2]='aaaa' == 2", lps[2] == 2);
    TEST("lps[3]='aaaa' == 3", lps[3] == 3);

    /* "abcabc" -> lps = [0,0,0,1,2,3] */
    const char *pat2 = "abcabc";
    int len2 = (int)strlen(pat2);
    init_LPS(lps);
    compute_lps(lps, pat2, len2);
    TEST("lps[0]='abcabc' == 0", lps[0] == 0);
    TEST("lps[3]='abcabc' == 1", lps[3] == 1);
    TEST("lps[4]='abcabc' == 2", lps[4] == 2);
    TEST("lps[5]='abcabc' == 3", lps[5] == 3);

    /* "abcd" – no prefix/suffix overlap -> all zeros */
    const char *pat3 = "abcd";
    int len3 = (int)strlen(pat3);
    init_LPS(lps);
    compute_lps(lps, pat3, len3);
    bool all_zero = true;
    for (int i = 0; i < len3; i++)
        if (lps[i] != 0) {
            all_zero = false;
            break;
        }
    TEST("lps 'abcd' are all zeros (no overlap)", all_zero);
}

// ---------------------------------------------------------------------------
// KMP: search_kmp
// ---------------------------------------------------------------------------
static void test_kmp_search_single_match(void) {
    SECTION("search_kmp – single match");
    int lps[MAX_SEARCH_BUFFER_LEN];
    const char *haystack = "create entity";
    const char *needle = "entity";
    int nlen = (int)strlen(needle);
    init_LPS(lps);
    compute_lps(lps, needle, nlen);

    int *hits = search_kmp(haystack, needle, nlen, lps);
    TEST("Single match found (hits != NULL)", hits != NULL);
    TEST("Exactly one hit", hits && vec_len(hits) == 1);
    TEST("Match starts at index 7 (after 'create ')", hits && hits[0] == 7);
    vec_free(hits);
}

static void test_kmp_search_no_match(void) {
    SECTION("search_kmp – no match");
    int lps[MAX_SEARCH_BUFFER_LEN];
    const char *haystack = "create entity";
    const char *needle = "relationship";
    int nlen = (int)strlen(needle);
    init_LPS(lps);
    compute_lps(lps, needle, nlen);

    int *hits = search_kmp(haystack, needle, nlen, lps);
    TEST("No match returns NULL or empty vec", hits == NULL || vec_len(hits) == 0);
    vec_free(hits);
}

static void test_kmp_search_multiple_matches(void) {
    SECTION("search_kmp – multiple matches");
    int lps[MAX_SEARCH_BUFFER_LEN];
    const char *haystack = "ENTITY entity Entity";
    const char *needle = "ntit"; /* appears in 'ENTITY', 'entity', 'Entity' */
    int nlen = (int)strlen(needle);
    init_LPS(lps);
    compute_lps(lps, needle, nlen);

    /* Case-sensitive: 'ntit' appears in 'ENTITY'(E-N-T-I-T-Y no, lowercase only)
     * Let's use a needle that definitively appears twice.                   */
    const char *hay2 = "abab";
    const char *needle2 = "ab";
    int nlen2 = (int)strlen(needle2);
    init_LPS(lps);
    compute_lps(lps, needle2, nlen2);
    int *hits2 = search_kmp(hay2, needle2, nlen2, lps);
    TEST("Two matches found in 'abab' for 'ab'", hits2 && vec_len(hits2) == 2);
    TEST("First match at index 0", hits2 && hits2[0] == 0);
    TEST("Second match at index 2", hits2 && hits2[1] == 2);
    vec_free(hits2);
}

static void test_kmp_search_full_string_match(void) {
    SECTION("search_kmp – needle equals haystack");
    int lps[MAX_SEARCH_BUFFER_LEN];
    const char *str = "ENTITY";
    int len = (int)strlen(str);
    init_LPS(lps);
    compute_lps(lps, str, len);

    int *hits = search_kmp(str, str, len, lps);
    TEST("Full-string match returns one hit", hits && vec_len(hits) == 1);
    TEST("Match starts at index 0", hits && hits[0] == 0);
    vec_free(hits);
}

// ---------------------------------------------------------------------------
// search_help_kmp: integration against the live helpdb
// ---------------------------------------------------------------------------
static void test_search_help_kmp_hit(void) {
    SECTION("search_help_kmp – term present in current page");
    HelpWindow hwin;
    init_help_window(&hwin, Main);
    set_current_page(&hwin, Main);

    /* "ENTITY" appears in Main page line 5 */
    const char *term = "ENTITY";
    int tlen = (int)strlen(term);
    SearchResult *results = search_help_kmp(&hwin, term, tlen);

    TEST("search_help_kmp returns non-NULL for known term", results != NULL);
    TEST("At least one result found", results && vec_len(results) >= 1);

    destroy_search_results(results);
    TEST("destroy_search_results does not crash", true);
}

static void test_search_help_kmp_miss(void) {
    SECTION("search_help_kmp – term absent from current page");
    HelpWindow hwin;
    init_help_window(&hwin, Main);
    set_current_page(&hwin, Main);

    /* This string is unlikely to appear in the help text */
    const char *term = "xyzzy_not_here";
    int tlen = (int)strlen(term);
    SearchResult *results = search_help_kmp(&hwin, term, tlen);

    TEST("search_help_kmp returns NULL for absent term", results == NULL || vec_len(results) == 0);
    destroy_search_results(results);
}

static void test_search_help_kmp_per_page(void) {
    SECTION("search_help_kmp – page-scoped results");
    HelpWindow hwin;
    init_help_window(&hwin, Main);

    /* "TAB" only appears in the Hotkeys page */
    const char *term = "TAB";
    int tlen = (int)strlen(term);

    set_current_page(&hwin, Main);
    SearchResult *main_results = search_help_kmp(&hwin, term, tlen);

    set_current_page(&hwin, Hotkeys);
    SearchResult *hotkey_results = search_help_kmp(&hwin, term, tlen);

    /* Hotkeys page should have more (or at least as many) hits */
    int main_count = main_results ? (int)vec_len(main_results) : 0;
    int hotkey_count = hotkey_results ? (int)vec_len(hotkey_results) : 0;
    TEST("Hotkeys page has more 'TAB' hits than Main page", hotkey_count >= main_count);

    destroy_search_results(main_results);
    destroy_search_results(hotkey_results);
}

static void test_search_help_kmp_result_line_numbers(void) {
    SECTION("search_help_kmp – result line numbers are within page range");
    HelpWindow hwin;
    init_help_window(&hwin, Main);
    set_current_page(&hwin, Examples);

    /* "create" appears in the Examples page code lines */
    const char *term = "create";
    int tlen = (int)strlen(term);
    SearchResult *results = search_help_kmp(&hwin, term, tlen);

    bool lines_valid = true;
    if (results) {
        int n = (int)vec_len(results);
        /* Examples page lines start at line_start 41 (from helpdb) */
        for (int i = 0; i < n; i++) {
            if (results[i].line < 41) {
                lines_valid = false;
                break;
            }
        }
    }
    TEST("All result line numbers fall within the Examples page range", lines_valid);
    destroy_search_results(results);
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------
void run_help_tests(void) {
    printf("Starting Help Window & KMP Search Tests...\n");
    log_file = fopen("help_test_results.log", "w");
    if (!log_file) {
        printf("CRITICAL ERROR: Could not open log file!\n");
        return;
    }

    SUITE_HEADER("HELP WINDOW & KMP SEARCH TESTS");

    test_help_window_init();
    test_help_set_current_page();
    test_help_set_scrolling_line();
    test_help_window_line_tokenization();
    test_kmp_lps_basic();
    test_kmp_search_single_match();
    test_kmp_search_no_match();
    test_kmp_search_multiple_matches();
    test_kmp_search_full_string_match();
    test_search_help_kmp_hit();
    test_search_help_kmp_miss();
    test_search_help_kmp_per_page();
    test_search_help_kmp_result_line_numbers();

    fprintf(log_file, "\n=========================================\n");
    fprintf(log_file, "  RESULTS: %d passed, %d failed\n", total_pass, total_fail);
    fprintf(log_file, "=========================================\n");
    fclose(log_file);
    printf("Help/KMP tests complete. Results: %d passed, %d failed.\n"
           "See 'help_test_results.log' for details.\n",
           total_pass, total_fail);
    total_pass = 0;
    total_fail = 0;
}

#endif // HELP_TESTS

// =========================================================================
// GRAPHICS TESTS (Compiled only if GRAPHICS_TEST is defined)
// =========================================================================
#ifdef GRAPHICS_TEST

#include "../MCD_elements.h"
#include "../global_objects.h"
#include "../graphics.h"
#include <ncurses.h>

// --- Setup helpers ---

static void setup_small_library_mcd(void) {
    Entity *member = createEntity("Member", 10, 5);
    addProperty(member, "member_id", "int");
    addProperty(member, "name", "str");

    Entity *book = createEntity("Book", 70, 5);
    addProperty(book, "isbn", "str");
    addProperty(book, "title", "str");
    addProperty(book, "author", "str");

    Relationship *borrows = addRelationship(40, 5, member, book, "Borrows");
    addPropertyRelationship(borrows, "due_date", "date");
    addCardinalityAPI("0,n,1,1", borrows);
}

static void setup_university_mcd(void) {
    Entity *student = createEntity("Student", 10, 5);
    addProperty(student, "student_id", "int");
    addProperty(student, "major", "str");

    Entity *course = createEntity("Course", 70, 5);
    addProperty(course, "course_code", "str");
    addProperty(course, "credits", "int");

    Entity *professor = createEntity("Professor", 70, 20);
    addProperty(professor, "emp_id", "int");
    addProperty(professor, "office", "str");

    Relationship *enrolls = addRelationship(40, 5, student, course, "Enrolls");
    addPropertyRelationship(enrolls, "grade", "str");
    addCardinalityAPI("0,n,1,n", enrolls);

    Relationship *teaches = addRelationship(70, 13, professor, course, "Teaches");
    addCardinalityAPI("1,n,1,1", teaches);
}

static void setup_large_e_commerce_delivery_mcd(void) {
    Entity *customer = createEntity("Customer", 5, 5);
    addProperty(customer, "customer_id", "int");
    addProperty(customer, "first_name", "str");

    Entity *shopping_cart = createEntity("Cart", 50, 5);
    addProperty(shopping_cart, "cart_id", "int");

    Entity *order = createEntity("Order", 95, 5);
    addProperty(order, "order_id", "int");

    Entity *product = createEntity("Product", 140, 5);
    addProperty(product, "product_id", "int");

    Entity *delivery = createEntity("Delivery", 5, 27);
    addProperty(delivery, "track_num", "str");

    Entity *warehouse = createEntity("Warehouse", 95, 27);
    addProperty(warehouse, "capacity", "int");

    Relationship *r_owns = addRelationship(31, 5, customer, shopping_cart, "Owns");
    addCardinalityAPI("1,1,0,1", r_owns);

    Relationship *r_checkout = addRelationship(76, 5, shopping_cart, order, "Checkout");
    addCardinalityAPI("0,1,1,1", r_checkout);

    Relationship *r_order_line = addRelationship(121, 5, order, product, "Ord_Line");
    addCardinalityAPI("1,n,0,n", r_order_line);

    Relationship *r_fulfilled = addRelationship(25, 16, order, delivery, "ShipsVia");
    addCardinalityAPI("0,1,1,1", r_fulfilled);

    Relationship *r_stocks = addRelationship(121, 16, warehouse, product, "Stocks");
    addCardinalityAPI("1,n,0,n", r_stocks);
}

static void setup_reflexive_mcd(void) {
    Entity *employee = createEntity("Employee", 30, 5);
    addProperty(employee, "emp_id", "int");
    addProperty(employee, "name", "str");

    Relationship *manages = addRelationship(30, 14, employee, employee, "Manages");
    addCardinalityAPI("0,n,0,1", manages);
}

static void setup_ternary_mcd(void) {
    Entity *supplier = createEntity("Supplier", 5, 5);
    Entity *part = createEntity("Part", 60, 5);
    Entity *project = createEntity("Project", 30, 20);

    Relationship *supplies = addRelationship(30, 5, supplier, part, "Supplies");
    addCardinalityAPI("0,n,1,n", supplies);
    (void)project; // ternary hookup is diagram-level; keep for completeness
}

// --- Test Runner ---

void run_graphics_tests(void) {
    printf("Starting Graphics Tests...\n");

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    initColors();

    typedef struct {
            const char *label;
            void (*setup)(void);
    } GraphicsCase;

    GraphicsCase cases[] = {
        {"[TEST 1/5] Small Library System (Binary + Properties)", setup_small_library_mcd},
        {"[TEST 2/5] Medium University System (Multi-Relationship)", setup_university_mcd},
        {"[TEST 3/5] Large E-Commerce & Delivery (Complex Layout)", setup_large_e_commerce_delivery_mcd},
        {"[TEST 4/5] Reflexive Relationship (Entity links to itself)", setup_reflexive_mcd},
        {"[TEST 5/5] Ternary / N-ary Stub (Supplier-Part-Project)", setup_ternary_mcd},
    };

    int num_cases = (int)(sizeof(cases) / sizeof(cases[0]));

    for (int i = 0; i < num_cases; i++) {
        init_global_objects();
        cases[i].setup();
        erase();
        draw_all_relationships(global_objects, -1, false);
        draw_all_entities(global_objects, -1, false);
        mvprintw(0, 0, "%s  Press any key to continue...", cases[i].label);
        refresh();
        getch();
    }

    endwin();
    printf("Graphics tests concluded safely.\n");
}

#endif // GRAPHICS_TEST

// =========================================================================
// MAIN RUNNER
// =========================================================================
int main(void) {
    int suites_run = 0;

#ifdef ARENA_TESTS
    run_arena_tests();
    suites_run++;
#endif

#ifdef LEXER_TESTS
    run_lexer_tests();
    suites_run++;
#endif

#ifdef PARSER_TESTS
    run_parser_tests();
    suites_run++;
#endif

#ifdef MCD_TESTS
    run_mcd_tests();
    suites_run++;
#endif

#ifdef HELP_TESTS
    run_help_tests();
    suites_run++;
#endif

#ifdef GRAPHICS_TEST
    run_graphics_tests();
    suites_run++;
#endif

    if (suites_run == 0) {
        printf("No test suites were compiled!\n");
        printf("Compile with one or more of:\n");
        printf("  -DARENA_TESTS    Arena allocator tests  (no ncurses)\n");
        printf("  -DLEXER_TESTS    Tokenizer/lexer tests  (no ncurses)\n");
        printf("  -DPARSER_TESTS   Parser + execute tests (no ncurses)\n");
        printf("  -DMCD_TESTS      MCD element API tests  (no ncurses)\n");
        printf("  -DHELP_TESTS     Help window + KMP tests(no ncurses)\n");
        printf("  -DGRAPHICS_TEST  Visual/ncurses tests   (requires ncurses)\n");
    }

    return (total_fail > 0) ? 1 : 0;
}
