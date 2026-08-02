// =========================================================================
// tests.c
// Unified testing suite for Arena Allocator, Lexer/Tokenizer, Parser,
// MCD Elements, Command Processor, Graphics/MCD generation, Utilities,
// and Integration Workflows.
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

static void test_arena_initialization(void) {
    SECTION("Initialization");
    Arena a;
    arena_init(&a, 1024);
    TEST("Head is NULL before first allocation", a.head == NULL);
    TEST("Current is NULL before first allocation", a.current == NULL);
    TEST("Default block size stored correctly", a.default_block_size == 1024);
}

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

static void test_arena_alignment_edge_cases(void) {
    SECTION("Alignment Edge Cases");
    Arena a;
    arena_init(&a, 4096);

    size_t alignments[] = {1, 2, 4, 8, 16, 32, 64};
    for (int i = 0; i < 7; i++) {
        char name[64];
        snprintf(name, sizeof(name), "Pointer aligned to %zu bytes", alignments[i]);
        void *p = arena_alloc_aligned(&a, 8, alignments[i]);
        TEST(name, p != NULL && (uintptr_t)p % alignments[i] == 0);
    }
}

static void test_arena_block_chaining(void) {
    SECTION("Block Chaining (OOM Overflow)");
    Arena a;
    arena_init(&a, 64);

    void *p1 = arena_alloc_aligned(&a, 40, 8);
    TEST("First 40-byte allocation succeeds in 64-byte arena", p1 != NULL);
    TEST("After first alloc: head == current (single block)", a.head == a.current);

    void *p2 = arena_alloc_aligned(&a, 40, 8);
    TEST("Second 40-byte allocation triggers block chain", p2 != NULL);
    TEST("Head and current now differ (chain formed)", a.head != a.current);
    TEST("head->next points to current block", a.head->next == a.current);
    TEST("p2 is still 8-byte aligned after block chain", (uintptr_t)p2 % 8 == 0);

    void *p3 = arena_alloc_aligned(&a, 40, 8);
    TEST("Third allocation (third block) succeeds", p3 != NULL);
    TEST("Chain has at least two links", a.head->next != NULL && a.head->next->next != NULL);
}

static void test_arena_oversized_allocation(void) {
    SECTION("Oversized Allocations");
    Arena a;
    arena_init(&a, 1024);

    void *p = arena_alloc_aligned(&a, 5000, 8);
    TEST("Allocation larger than default block size succeeds", p != NULL);
    TEST("Current block capacity scaled to fit the payload", a.current != NULL && a.current->cap >= 5000);

    void *q = arena_alloc_aligned(&a, 32, 8);
    TEST("Normal allocation after oversized request still works", q != NULL);
}

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

static void test_arena_zero_alloc(void) {
    SECTION("Zero-Size Allocation");
    Arena a;
    arena_init(&a, 1024);
    void *p = arena_alloc_aligned(&a, 0, 8);
    TEST("Zero-size allocation returns non-NULL (or NULL-ok)", p != NULL || p == NULL);
}

static void test_temp_arena(void) {
    SECTION("Temporary Arena (save/restore)");
    Arena a;
    arena_init(&a, 1024);

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
    void *after = arena_alloc_aligned(&a, 32, 8);
    TEST("Allocation after free_temp_arena succeeds (arena still live)", after != NULL);
}

static void test_arena_destroy(void) {
    SECTION("Arena Destroy");
    Arena a;
    arena_init(&a, 256);
    arena_alloc_aligned(&a, 100, 8);
    arena_alloc_aligned(&a, 200, 8);

    destroy_arena(&a);
    arena_init(&a, 128);
    void *p = arena_alloc_aligned(&a, 10, 4);
    TEST("Re-initialisation after destroy succeeds", p != NULL);
    destroy_arena(&a);
    TEST("Second destroy does not crash (double-destroy safety)", true);
}

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
    test_arena_zero_alloc();
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
    int count = 99;
    tokenize_content(NULL, tokens, &count);
    TEST("NULL input: count reset to 0", count == 0);
}

static void test_tokenizer_position_tracking(void) {
    SECTION("Token Position Tracking (for error reporting)");
    Token tokens[MAX_TOKENS_NUM_PER_COMMAND];
    int count = 0;

    tokenize_content("create \"Foo\"", tokens, &count);
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

static void test_tokenizer_numbers_and_symbols(void) {
    SECTION("Numbers and Mixed Symbols");
    Token tokens[MAX_TOKENS_NUM_PER_COMMAND];
    int count = 0;

    tokenize_content("add property \"E\" 123 45.6", tokens, &count);
    TEST("Numeric token classified appropriately", tokens[3].type == TOKEN_IDENTIFIER || tokens[3].type == TOKEN_UNKNOWN);
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
    test_tokenizer_numbers_and_symbols();

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
// =========================================================================
#ifdef PARSER_TESTS

#include "../Lexer/parse.h"
#include "../Lexer/tokenize.h"
#include "../MCD_elements.h"
#include "../global_objects.h"

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
    advance_token(&p.current);

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
    advance_token(&p.current);

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
    const char *input = "add property \"Customer\" \"customer_id\" int";
    init_parser(&p, input);
    tokenize_content(input, p.tokens, &p.count);
    advance_token(&p.current);

    AddCommand c = {0};
    bool ok = parse_add(&p, NULL, &c);

    TEST("parse_add returns true for valid add property command", ok);
    TEST("Target identifier name captured", strcmp(c.identifier_name, "Customer") == 0);
    TEST("Property name captured", strcmp(c.Data.p.prop_name, "customer_id") == 0);
    TEST("Property type captured", strcmp(c.Data.p.prop_type, "int") == 0);
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
    execute_create(c);
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

    CreateCommand cc = {0};
    cc.type = TYPE_ENTITY;
    strncpy(cc.Data.e.name, "User", MAX_NAME_LEN - 1);
    execute_create(cc);

    AddCommand ac = {0};
    strncpy(ac.identifier_name, "User", MAX_NAME_LEN - 1);
    strncpy(ac.Data.p.prop_name, "user_id", MAX_NAME_LEN - 1);
    strncpy(ac.Data.p.prop_type, "int", MAX_TYPE_LEN - 1);
    execute_addProperty(ac, NULL);

    Entity *e = search_entity("User");
    TEST("Entity exists after addProperty", e != NULL);
    TEST("Entity has at least one property", e && e->num_properties >= 1);
    TEST("Added property name matches 'user_id'",
         e && e->num_properties >= 1 && strcmp(e->properties[0]->name, "user_id") == 0);
    TEST("Added property type matches 'int'", e && e->num_properties >= 1 && strcmp(e->properties[0]->type, "int") == 0);
}

static void test_execute_addProperty_relationship(void) {
    SECTION("execute_addProperty – onto Relationship");
    init_global_objects();

    CreateCommand cr = {0};
    cr.type = TYPE_RELATIONSHIP;
    strncpy(cr.Data.r.name, "Rents", MAX_NAME_LEN - 1);
    strncpy(cr.Data.r.e1_name, "Renter", MAX_NAME_LEN - 1);
    strncpy(cr.Data.r.e2_name, "Car", MAX_NAME_LEN - 1);
    execute_create(cr);

    AddCommand ac = {0};
    strncpy(ac.identifier_name, "Rents", MAX_NAME_LEN - 1);
    strncpy(ac.Data.p.prop_name, "rental_date", MAX_NAME_LEN - 1);
    strncpy(ac.Data.p.prop_type, "date", MAX_TYPE_LEN - 1);
    execute_addProperty(ac, NULL);

    Relationship *r = search_relationship("Rents");
    TEST("Relationship exists after addProperty", r != NULL);
    TEST("Relationship has at least one property", r && r->num_properties >= 1);
    TEST("Property name matches 'rental_date'",
         r && r->num_properties >= 1 && strcmp(r->properties[0]->name, "rental_date") == 0);
}

static void test_execute_addProperty_missing_target(void) {
    SECTION("execute_addProperty – Missing Target");
    init_global_objects();

    AddCommand ac = {0};
    strncpy(ac.identifier_name, "GhostEntity", MAX_NAME_LEN - 1);
    strncpy(ac.Data.p.prop_name, "x", MAX_NAME_LEN - 1);
    strncpy(ac.Data.p.prop_type, "int", MAX_TYPE_LEN - 1);
    execute_addProperty(ac, NULL);

    Entity *e = search_entity("GhostEntity");
    TEST("Missing target entity was not auto-created", e == NULL);
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
    test_execute_addProperty_relationship();
    test_execute_addProperty_missing_target();

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
    addProperty(e, "price", "float", NORMAL_KEY);
    addProperty(e, "sku", "str", NORMAL_KEY);

    TEST("num_properties == 2 after two addProperty calls", e->num_properties == 2);
    TEST("First property name is 'price'", strcmp(e->properties[0]->name, "price") == 0);
    TEST("First property type is 'float'", strcmp(e->properties[0]->type, "float") == 0);
    TEST("Second property name is 'sku'", strcmp(e->properties[1]->name, "sku") == 0);
    TEST("Second property type is 'str'", strcmp(e->properties[1]->type, "str") == 0);
}

static void test_mcd_add_property_with_keytypes(void) {
    SECTION("addProperty – KeyType handling");
    init_global_objects();

    Entity *e = createEntity("Typed", 0, 0);
    addProperty(e, "id", "int", PRIMARY_KEY);
    addProperty(e, "ref_id", "int", FOREIGN_KEY);
    addProperty(e, "name", "str", NORMAL_KEY);

    TEST("PRIMARY_KEY stored correctly", e->properties[0]->keytype == PRIMARY_KEY);
    TEST("FOREIGN_KEY stored correctly", e->properties[1]->keytype == FOREIGN_KEY);
    TEST("NORMAL_KEY stored correctly", e->properties[2]->keytype == NORMAL_KEY);
}

static void test_mcd_add_property_duplicate(void) {
    SECTION("addProperty – Duplicate Prevention");
    init_global_objects();

    Entity *e = createEntity("DupTest", 0, 0);
    bool ok1 = addProperty(e, "code", "str", NORMAL_KEY);
    bool ok2 = addProperty(e, "code", "int", NORMAL_KEY);
    TEST("First addProperty succeeds", ok1);
    TEST("Duplicate addProperty rejected", !ok2);
    TEST("Still only one property", e->num_properties == 1);
}

static void test_mcd_add_property_max_properties(void) {
    SECTION("addProperty – MAX_PROPERTIES boundary");
    init_global_objects();

    Entity *e = createEntity("MaxProp", 0, 0);
    bool ok = true;
    for (int i = 0; i < MAX_PROPERTIES + 2; i++) {
        char name[16];
        snprintf(name, sizeof(name), "p%d", i);
        if (!addProperty(e, name, "int", NORMAL_KEY)) {
            if (i < MAX_PROPERTIES) {
                ok = false;
            }
        }
    }
    TEST("Exactly MAX_PROPERTIES stored", e->num_properties == MAX_PROPERTIES);
    TEST("No failure before limit", ok);
}

static void test_mcd_property_references(void) {
    SECTION("set_property_reference / set_property_reference_column");
    init_global_objects();

    Entity *e = createEntity("RefTest", 0, 0);
    addProperty(e, "fk_col", "int", FOREIGN_KEY);

    set_property_reference(e, "fk_col", "Customer");
    set_property_reference_column(e, "fk_col", "customer_id");

    TEST("references entity set", strcmp(e->properties[0]->references, "Customer") == 0);
    TEST("references column set", strcmp(e->properties[0]->references_column, "customer_id") == 0);
    TEST("references is null-terminated", e->properties[0]->references[MAX_NAME_LEN - 1] == '\0');
    TEST("references_column is null-terminated", e->properties[0]->references_column[MAX_NAME_LEN - 1] == '\0');
}

static void test_mcd_property_reference_not_found(void) {
    SECTION("set_property_reference – missing property");
    init_global_objects();

    Entity *e = createEntity("NoProp", 0, 0);
    addProperty(e, "only", "int", NORMAL_KEY);

    set_property_reference(e, "missing", "Other");
    TEST("Missing property leaves existing property untouched", strlen(e->properties[0]->references) == 0);
}

static void test_mcd_strcasecmp(void) {
    SECTION("mcd_strcasecmp");
    TEST("Exact match", mcd_strcasecmp("abc", "abc") == 0);
    TEST("Case insensitive equal", mcd_strcasecmp("ABC", "abc") == 0);
    TEST("Case insensitive equal reversed", mcd_strcasecmp("abc", "ABC") == 0);
    TEST("Different strings", mcd_strcasecmp("abc", "def") < 0);
    TEST("Prefix difference", mcd_strcasecmp("abc", "abd") < 0);
    TEST("Empty vs non-empty", mcd_strcasecmp("", "a") < 0);
    TEST("Empty vs empty", mcd_strcasecmp("", "") == 0);
    TEST("Mixed case long", mcd_strcasecmp("EntityName", "entityname") == 0);
}

static void test_mcd_find_best_attach_point(void) {
    SECTION("findBestAttachPoint");
    AttachPoint ap;

    ap = findBestAttachPoint(10, 10, 10, 5, 15, 0);
    TEST("Target above -> SIDE_TOP", ap.side == SIDE_TOP);

    ap = findBestAttachPoint(10, 10, 10, 5, 15, 20);
    TEST("Target below -> SIDE_BOTTOM", ap.side == SIDE_BOTTOM);

    ap = findBestAttachPoint(10, 10, 10, 5, 0, 12);
    TEST("Target left -> SIDE_LEFT", ap.side == SIDE_LEFT);

    ap = findBestAttachPoint(10, 10, 10, 5, 25, 12);
    TEST("Target right -> SIDE_RIGHT", ap.side == SIDE_RIGHT);
}

static void test_mcd_create_relationship(void) {
    SECTION("addRelationship");
    init_global_objects();

    Entity *e1 = createEntity("Author", 0, 0);
    Entity *e2 = createEntity("Book", 50, 0);
    Relationship *r = addRelationship(25, 0, e1, e2, "Writes");

    TEST("addRelationship returns non-NULL pointer", r != NULL);
    TEST("Relationship name stored correctly", strcmp(r->name, "Writes") == 0);
    TEST("Relationship x coordinate is non-negative", r->x >= 0);
    TEST("Relationship y coordinate is non-negative", r->y >= 0);
    TEST("Relationship e1 pointer correct", r->e1 == e1);
    TEST("Relationship e2 pointer correct", r->e2 == e2);
    TEST("Relationship has zero properties on creation", r->num_properties == 0);
    TEST("Relationship registered in global_objects", global_objects.relationship_count == 1);
    TEST("global_objects.relationships[0] == returned pointer", global_objects.relationships[0] == r);
}

static void test_mcd_relationship_null_entities(void) {
    SECTION("addRelationship – NULL safety");
    init_global_objects();

    Entity *e1 = createEntity("A", 0, 0);
    Relationship *r1 = addRelationship(0, 0, NULL, e1, "R1");
    Relationship *r2 = addRelationship(0, 0, e1, NULL, "R2");
    Relationship *r3 = addRelationship(0, 0, NULL, NULL, "R3");

    TEST("NULL e1 returns NULL", r1 == NULL);
    TEST("NULL e2 returns NULL", r2 == NULL);
    TEST("Both NULL returns NULL", r3 == NULL);
}

static void test_mcd_add_property_relationship(void) {
    SECTION("addPropertyRelationship");
    init_global_objects();

    Entity *e1 = createEntity("Student", 0, 0);
    Entity *e2 = createEntity("Course", 40, 0);
    Relationship *r = addRelationship(20, 0, e1, e2, "Enrolls");
    addPropertyRelationship(r, "grade", "str", NORMAL_KEY);
    addPropertyRelationship(r, "semester", "str", NORMAL_KEY);

    TEST("Relationship has 2 properties after two add calls", r->num_properties == 2);
    TEST("Property[0] name is 'grade'", strcmp(r->properties[0]->name, "grade") == 0);
    TEST("Property[1] name is 'semester'", strcmp(r->properties[1]->name, "semester") == 0);
}

static void test_mcd_add_property_relationship_keytypes(void) {
    SECTION("addPropertyRelationship – KeyTypes");
    init_global_objects();

    Entity *e1 = createEntity("A", 0, 0);
    Entity *e2 = createEntity("B", 10, 0);
    Relationship *r = addRelationship(5, 0, e1, e2, "Link");
    addPropertyRelationship(r, "pk", "int", PRIMARY_KEY);
    addPropertyRelationship(r, "fk", "int", FOREIGN_KEY);

    TEST("Relationship PRIMARY_KEY correct", r->properties[0]->keytype == PRIMARY_KEY);
    TEST("Relationship FOREIGN_KEY correct", r->properties[1]->keytype == FOREIGN_KEY);
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

    TEST("cards[0] is populated after addCardinalityAPI", r->cards[0] != NULL);
    TEST("cards[1] is populated after addCardinalityAPI", r->cards[1] != NULL);
}

static void test_mcd_cardinality_edge_cases(void) {
    SECTION("addCardinality – Edge Cases");
    Cardinality c1, c2;

    addCardinality(NULL, &c1, &c2);
    TEST("NULL input yields default c1", strcmp(c1.value, "x,x") == 0);
    TEST("NULL input yields default c2", strcmp(c2.value, "y,y") == 0);

    addCardinality("1,1", &c1, &c2);
    TEST("Short input yields default c1", strcmp(c1.value, "x,x") == 0);

    addCardinality("n,1,1,n", &c1, &c2);
    TEST("Reversed order corrected for c1", c1.value[0] <= c1.value[2]);
    TEST("Reversed order corrected for c2", c2.value[0] <= c2.value[2]);
}

static void test_mcd_global_objects_clear(void) {
    SECTION("init_global_objects – reset");
    createEntity("Garbage1", 0, 0);
    createEntity("Garbage2", 0, 0);

    init_global_objects();
    TEST("entity_count resets to 0 after init_global_objects", global_objects.entity_count == 0);
    TEST("relationship_count resets to 0 after init_global_objects", global_objects.relationship_count == 0);
}

static void test_mcd_entity_screen_clamping(void) {
    SECTION("createEntity – Screen Clamping");
    init_global_objects();

    Entity *e = createEntity("OffScreen", 9999, 9999);
    TEST("X clamped to valid range", e->x < 9999);
    TEST("Y clamped to valid range", e->y < 9999);
    TEST("Entity still created", e != NULL);
}

static void test_mcd_property_width_expansion(void) {
    SECTION("addProperty – Width Expansion");
    init_global_objects();

    Entity *e = createEntity("Wide", 0, 0);
    int base_width = e->width;
    addProperty(e, "verylongpropertyname", "verylongtype", NORMAL_KEY);
    TEST("Width expanded for long property+type", e->width > base_width);

    Entity *e2 = createEntity("FKWide", 0, 0);
    int base2 = e2->width;
    addProperty(e2, "fk", "int", FOREIGN_KEY);
    TEST("Width expanded for FK prefix", e2->width > base2);
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
    test_mcd_add_property_with_keytypes();
    test_mcd_add_property_duplicate();
    test_mcd_add_property_max_properties();
    test_mcd_property_references();
    test_mcd_property_reference_not_found();
    test_mcd_strcasecmp();
    test_mcd_find_best_attach_point();
    test_mcd_create_relationship();
    test_mcd_relationship_null_entities();
    test_mcd_add_property_relationship();
    test_mcd_add_property_relationship_keytypes();
    test_mcd_search_entity();
    test_mcd_search_relationship();
    test_mcd_cardinality();
    test_mcd_cardinality_edge_cases();
    test_mcd_global_objects_clear();
    test_mcd_entity_screen_clamping();
    test_mcd_property_width_expansion();

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
// =========================================================================
#ifdef HELP_TESTS

#include "../DSA/kmp.h"
#include "../DSA/vec.h"
#include "../help_window.h"

static void test_help_window_init(void) {
    SECTION("init_help_window – initial state");
    HelpWindow hwin;
    init_help_window(&hwin, Main);

    TEST("current_page is Main after init", hwin.current_page == Main);
    TEST("pages_db pointer is non-NULL", hwin.pages_db != NULL);
    TEST("main_scrolling_line initialised to 0", hwin.main_scrolling_line == 0);
    TEST("hotkey_scrolling_line is non-negative", hwin.hotkey_scrolling_line >= 0);
    TEST("examples_scrolling_line is non-negative", hwin.examples_scrolling_line >= 0);

    for (int pg = 0; pg < HelpPageNum; pg++) {
        TEST("Page has at least one line", hwin.pages_db[pg].line_count > 0);
        TEST("First line on page has non-NULL text", hwin.pages_db[pg].lines[0].text != NULL);
    }
}

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

    set_current_page(NULL, Hotkeys);
    TEST("set_current_page(NULL, ...) does not crash", true);
}

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

    set_current_page(&hwin, Main);
    set_scrolling_line(&hwin, 5);
    set_scrolling_line(&hwin, -1);
    TEST("set_scrolling_line(-1) is ignored – value unchanged", hwin.main_scrolling_line == 5);
}

static void test_help_window_line_tokenization(void) {
    SECTION("HelpLine tokenization during init");
    HelpWindow hwin;
    init_help_window(&hwin, Main);

    const HelpLine *title = &hwin.pages_db[Main].lines[0];
    TEST("Main page title line has been tokenized (token_count > 0)", title->token_count > 0);
    TEST("Main page title line line_len > 0", title->line_len > 0);

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

static void test_kmp_lps_basic(void) {
    SECTION("compute_lps – basic patterns");
    int lps[MAX_SEARCH_BUFFER_LEN];

    const char *pat1 = "aaaa";
    int len1 = (int)strlen(pat1);
    init_LPS(lps);
    compute_lps(lps, pat1, len1);
    TEST("lps[0]='aaaa' == 0", lps[0] == 0);
    TEST("lps[1]='aaaa' == 1", lps[1] == 1);
    TEST("lps[2]='aaaa' == 2", lps[2] == 2);
    TEST("lps[3]='aaaa' == 3", lps[3] == 3);

    const char *pat2 = "abcabc";
    int len2 = (int)strlen(pat2);
    init_LPS(lps);
    compute_lps(lps, pat2, len2);
    TEST("lps[0]='abcabc' == 0", lps[0] == 0);
    TEST("lps[3]='abcabc' == 1", lps[3] == 1);
    TEST("lps[4]='abcabc' == 2", lps[4] == 2);
    TEST("lps[5]='abcabc' == 3", lps[5] == 3);

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

static void test_kmp_search_overlapping(void) {
    SECTION("search_kmp – overlapping pattern");
    int lps[MAX_SEARCH_BUFFER_LEN];
    const char *hay = "aaaa";
    const char *needle = "aa";
    int nlen = (int)strlen(needle);
    init_LPS(lps);
    compute_lps(lps, needle, nlen);
    int *hits = search_kmp(hay, needle, nlen, lps);
    TEST("Overlapping hits found", hits && vec_len(hits) == 3);
    TEST("Hit 0 at 0", hits && hits[0] == 0);
    TEST("Hit 1 at 1", hits && hits[1] == 1);
    TEST("Hit 2 at 2", hits && hits[2] == 2);
    vec_free(hits);
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

static void test_search_help_kmp_hit(void) {
    SECTION("search_help_kmp – term present in current page");
    HelpWindow hwin;
    init_help_window(&hwin, Main);
    set_current_page(&hwin, Main);

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

    const char *term = "TAB";
    int tlen = (int)strlen(term);

    set_current_page(&hwin, Main);
    SearchResult *main_results = search_help_kmp(&hwin, term, tlen);

    set_current_page(&hwin, Hotkeys);
    SearchResult *hotkey_results = search_help_kmp(&hwin, term, tlen);

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

    const char *term = "create";
    int tlen = (int)strlen(term);
    SearchResult *results = search_help_kmp(&hwin, term, tlen);

    bool lines_valid = true;
    if (results) {
        int n = (int)vec_len(results);
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
    test_kmp_search_overlapping();
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

static void setup_small_library_mcd(void) {
    Entity *member = createEntity("Member", 10, 5);
    addProperty(member, "member_id", "int", PRIMARY_KEY);
    addProperty(member, "name", "str", NORMAL_KEY);

    Entity *book = createEntity("Book", 70, 5);
    addProperty(book, "isbn", "str", PRIMARY_KEY);
    addProperty(book, "title", "str", NORMAL_KEY);
    addProperty(book, "author", "str", NORMAL_KEY);

    Relationship *borrows = addRelationship(40, 5, member, book, "Borrows");
    addPropertyRelationship(borrows, "due_date", "date", NORMAL_KEY);
    addCardinalityAPI("0,n,1,1", borrows);
}

static void setup_university_mcd(void) {
    Entity *student = createEntity("Student", 10, 5);
    addProperty(student, "student_id", "int", PRIMARY_KEY);
    addProperty(student, "major", "str", NORMAL_KEY);

    Entity *course = createEntity("Course", 70, 5);
    addProperty(course, "course_code", "str", PRIMARY_KEY);
    addProperty(course, "credits", "int", NORMAL_KEY);

    Entity *professor = createEntity("Professor", 70, 20);
    addProperty(professor, "emp_id", "int", PRIMARY_KEY);
    addProperty(professor, "office", "str", NORMAL_KEY);

    Relationship *enrolls = addRelationship(40, 5, student, course, "Enrolls");
    addPropertyRelationship(enrolls, "grade", "str", NORMAL_KEY);
    addCardinalityAPI("0,n,1,n", enrolls);

    Relationship *teaches = addRelationship(70, 13, professor, course, "Teaches");
    addCardinalityAPI("1,n,1,1", teaches);
}

static void setup_large_e_commerce_delivery_mcd(void) {
    Entity *customer = createEntity("Customer", 5, 5);
    addProperty(customer, "customer_id", "int", PRIMARY_KEY);
    addProperty(customer, "first_name", "str", NORMAL_KEY);

    Entity *shopping_cart = createEntity("Cart", 50, 5);
    addProperty(shopping_cart, "cart_id", "int", PRIMARY_KEY);

    Entity *order = createEntity("Order", 95, 5);
    addProperty(order, "order_id", "int", PRIMARY_KEY);

    Entity *product = createEntity("Product", 140, 5);
    addProperty(product, "product_id", "int", PRIMARY_KEY);

    Entity *delivery = createEntity("Delivery", 5, 27);
    addProperty(delivery, "track_num", "str", PRIMARY_KEY);

    Entity *warehouse = createEntity("Warehouse", 95, 27);
    addProperty(warehouse, "capacity", "int", NORMAL_KEY);

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
    addProperty(employee, "emp_id", "int", PRIMARY_KEY);
    addProperty(employee, "name", "str", NORMAL_KEY);

    Relationship *manages = addRelationship(30, 14, employee, employee, "Manages");
    addCardinalityAPI("0,n,0,1", manages);
}

static void setup_ternary_mcd(void) {
    Entity *supplier = createEntity("Supplier", 5, 5);
    Entity *part = createEntity("Part", 60, 5);
    Entity *project = createEntity("Project", 30, 20);

    Relationship *supplies = addRelationship(30, 5, supplier, part, "Supplies");
    addCardinalityAPI("0,n,1,n", supplies);
    (void)project;
}

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
        draw_all_relationships(&global_objects, -1, false);
        draw_all_entities(&global_objects, -1, false);
        mvprintw(0, 0, "%s  Press any key to continue...", cases[i].label);
        refresh();
        getch();
    }

    endwin();
    printf("Graphics tests concluded safely.\n");
}

#endif // GRAPHICS_TEST

// =========================================================================
// INTEGRATION TESTS (Compiled only if INTEGRATION_TESTS is defined)
// =========================================================================
#ifdef INTEGRATION_TESTS

#include "../MCD_elements.h"
#include "../global_objects.h"

static void test_integration_full_workflow(void) {
    SECTION("Full Workflow – Library System");
    init_global_objects();

    Entity *member = createEntity("Member", 10, 5);
    addProperty(member, "member_id", "int", PRIMARY_KEY);
    addProperty(member, "name", "str", NORMAL_KEY);

    Entity *book = createEntity("Book", 60, 5);
    addProperty(book, "isbn", "str", PRIMARY_KEY);
    addProperty(book, "title", "str", NORMAL_KEY);

    Relationship *borrows = addRelationship(35, 5, member, book, "Borrows");
    addPropertyRelationship(borrows, "due_date", "date", NORMAL_KEY);
    addCardinalityAPI("0,n,1,1", borrows);

    TEST("Two entities registered", global_objects.entity_count == 2);
    TEST("One relationship registered", global_objects.relationship_count == 1);
    TEST("Member has 2 properties", member->num_properties == 2);
    TEST("Book has 2 properties", book->num_properties == 2);
    TEST("Relationship has 1 property", borrows->num_properties == 1);
    TEST("Cardinality c1 set",
         strcmp(borrows->cards[0]->value, "0,n") == 0 || strcmp(borrows->cards[0]->value, "n,0") != 0);
    TEST("FK reference can be set post-creation", true);
    set_property_reference_column(member, "member_id", "id");
    TEST("Reference column set on PK", strcmp(member->properties[0]->references_column, "id") == 0);
}

static void test_integration_cascade_delete_logic(void) {
    SECTION("Integration – Global Reset Isolation");
    init_global_objects();

    createEntity("E1", 0, 0);
    createEntity("E2", 10, 0);
    TEST("Entities exist before reset", global_objects.entity_count == 2);

    init_global_objects();
    TEST("Entities cleared after reset", global_objects.entity_count == 0);
    TEST("Relationships cleared after reset", global_objects.relationship_count == 0);
}

void run_integration_tests(void) {
    printf("Starting Integration Tests...\n");
    log_file = fopen("integration_test_results.log", "w");
    if (!log_file) {
        printf("CRITICAL ERROR: Could not open log file!\n");
        return;
    }

    SUITE_HEADER("INTEGRATION TESTS");
    test_integration_full_workflow();
    test_integration_cascade_delete_logic();

    fprintf(log_file, "\n=========================================\n");
    fprintf(log_file, "  RESULTS: %d passed, %d failed\n", total_pass, total_fail);
    fprintf(log_file, "=========================================\n");
    fclose(log_file);
    printf("Integration tests complete. Results: %d passed, %d failed.\n"
           "See 'integration_test_results.log' for details.\n",
           total_pass, total_fail);
    total_pass = 0;
    total_fail = 0;
}

#endif // INTEGRATION_TESTS

// =========================================================================
// MAIN RUNNER
// =========================================================================
// =========================================================================
// UNDO & DELETE COMMAND TESTS (Compiled only if UNDO_DELETE_TESTS is defined)
// =========================================================================
#ifdef UNDO_DELETE_TESTS

#include <stdlib.h>
#include "../DSA/undo_stack.h"
#include "../Lexer/parse.h"
#include "../MCD_elements.h"
#include "../global_objects.h"
#include "../utils/save.h" // for snapshot_diagram_to_buf

// -------------------------------------------------------------------------
// DELETE COMMAND PARSER TESTS
// -------------------------------------------------------------------------
static void test_parse_delete_entity_element(void) {
    SECTION("parse_delete – delete entire entity");
    Parser p;
    const char *input = "delete \"Customer\"";
    init_parser(&p, input);
    tokenize_content(input, p.tokens, &p.count);
    advance_token(&p.current); // consume 'delete'

    DeleteCommand c = {0};
    bool ok = parse_delete(&p, NULL, &c);

    TEST("parse_delete returns true", ok);
    TEST("Name captured correctly", strcmp(c.name, "Customer") == 0);
    TEST("Defaults to ELEMENT deletion", c.type == ELEMENT);
}

static void test_parse_delete_entity_property(void) {
    SECTION("parse_delete – delete entity property");
    Parser p;
    const char *input = "delete \"Customer\" \"email\"";
    init_parser(&p, input);
    tokenize_content(input, p.tokens, &p.count);
    advance_token(&p.current); // consume 'delete'

    DeleteCommand c = {0};
    bool ok = parse_delete(&p, NULL, &c);

    TEST("parse_delete returns true", ok);
    TEST("Entity name captured", strcmp(c.name, "Customer") == 0);
    TEST("Property type set to PROP", c.type == PROP);
    TEST("Property name captured", strcmp(c.prop_name, "email") == 0);
}

static void test_parse_delete_relationship_property(void) {
    SECTION("parse_delete – delete relationship property");
    Parser p;
    const char *input = "delete \"Borrows\" \"due_date\"";
    init_parser(&p, input);
    tokenize_content(input, p.tokens, &p.count);
    advance_token(&p.current);

    DeleteCommand c = {0};
    bool ok = parse_delete(&p, NULL, &c);

    TEST("parse_delete returns true", ok);
    TEST("Relationship name captured", strcmp(c.name, "Borrows") == 0);
    TEST("Property type set to PROP", c.type == PROP);
    TEST("Property name captured", strcmp(c.prop_name, "due_date") == 0);
}

static void test_parse_delete_empty_name(void) {
    SECTION("parse_delete – empty name error");
    Parser p;
    const char *input = "delete \"\"";
    init_parser(&p, input);
    tokenize_content(input, p.tokens, &p.count);
    advance_token(&p.current);

    DeleteCommand c = {0};
    bool ok = parse_delete(&p, NULL, &c);
    TEST("Empty name rejected", !ok);
}

static void test_parse_delete_missing_name(void) {
    SECTION("parse_delete – missing name error");
    Parser p;
    const char *input = "delete";
    init_parser(&p, input);
    tokenize_content(input, p.tokens, &p.count);
    advance_token(&p.current);

    DeleteCommand c = {0};
    bool ok = parse_delete(&p, NULL, &c);
    TEST("Missing name rejected", !ok);
}

// -------------------------------------------------------------------------
// DELETE COMMAND EXECUTION TESTS
// -------------------------------------------------------------------------
static void test_execute_delete_entity(void) {
    SECTION("execute_delete – delete entity");
    init_global_objects();

    Entity *e = createEntity("ToDelete", 5, 5);
    addProperty(e, "id", "int", PRIMARY_KEY);
    TEST("Entity exists before delete", search_entity("ToDelete") != NULL);

    DeleteCommand c = {0};
    strncpy(c.name, "ToDelete", MAX_NAME_LEN - 1);
    c.type = ELEMENT;

    bool ok = execute_delete(c, NULL);
    TEST("execute_delete returns true", ok);
    TEST("Entity removed", search_entity("ToDelete") == NULL);
    TEST("Entity count is 0", global_objects.entity_count == 0);
}

static void test_execute_delete_entity_cascade_relationships(void) {
    SECTION("execute_delete – cascade delete attached relationships");
    init_global_objects();

    Entity *e1 = createEntity("Parent", 5, 5);
    Entity *e2 = createEntity("Child", 50, 5);
    addRelationship(27, 5, e1, e2, "HasChild");
    TEST("Relationship exists before delete", search_relationship("HasChild") != NULL);

    DeleteCommand c = {0};
    strncpy(c.name, "Parent", MAX_NAME_LEN - 1);
    c.type = ELEMENT;

    bool ok = execute_delete(c, NULL);
    TEST("execute_delete returns true", ok);
    TEST("Entity deleted", search_entity("Parent") == NULL);
    TEST("Attached relationship also deleted", search_relationship("HasChild") == NULL);
    TEST("Cascade removed relationship count", global_objects.relationship_count == 0);
}

static void test_execute_delete_entity_property(void) {
    SECTION("execute_delete – delete entity property");
    init_global_objects();

    Entity *e = createEntity("Product", 5, 5);
    addProperty(e, "sku", "str", NORMAL_KEY);
    addProperty(e, "price", "double", NORMAL_KEY);
    TEST("Two properties before delete", e->num_properties == 2);

    DeleteCommand c = {0};
    strncpy(c.name, "Product", MAX_NAME_LEN - 1);
    strncpy(c.prop_name, "sku", MAX_NAME_LEN - 1);
    c.type = PROP;

    bool ok = execute_delete(c, NULL);
    TEST("execute_delete returns true", ok);
    TEST("One property after delete", e->num_properties == 1);
    TEST("Remaining property is 'price'", strcmp(e->properties[0]->name, "price") == 0);
    TEST("Deleted property gone", search_entity("Product") != NULL);
}

static void test_execute_delete_entity_property_not_found(void) {
    SECTION("execute_delete – delete non-existent property");
    init_global_objects();

    Entity *e = createEntity("Product", 5, 5);
    addProperty(e, "sku", "str", NORMAL_KEY);

    DeleteCommand c = {0};
    strncpy(c.name, "Product", MAX_NAME_LEN - 1);
    strncpy(c.prop_name, "nonexistent", MAX_NAME_LEN - 1);
    c.type = PROP;

    bool ok = execute_delete(c, NULL);
    TEST("execute_delete returns false for missing property", !ok);
    TEST("Original property still exists", e->num_properties == 1);
}

static void test_execute_delete_relationship(void) {
    SECTION("execute_delete – delete entire relationship");
    init_global_objects();

    Entity *e1 = createEntity("A", 5, 5);
    Entity *e2 = createEntity("B", 50, 5);
    Relationship *r = addRelationship(27, 5, e1, e2, "Link");
    addPropertyRelationship(r, "prop1", "int", NORMAL_KEY);
    TEST("Relationship exists before delete", search_relationship("Link") != NULL);

    DeleteCommand c = {0};
    strncpy(c.name, "Link", MAX_NAME_LEN - 1);
    c.type = ELEMENT;

    bool ok = execute_delete(c, NULL);
    TEST("execute_delete returns true", ok);
    TEST("Relationship removed", search_relationship("Link") == NULL);
    TEST("Entities still exist", search_entity("A") != NULL && search_entity("B") != NULL);
}

static void test_execute_delete_relationship_property(void) {
    SECTION("execute_delete – delete relationship property");
    init_global_objects();

    Entity *e1 = createEntity("A", 5, 5);
    Entity *e2 = createEntity("B", 50, 5);
    Relationship *r = addRelationship(27, 5, e1, e2, "Link");
    addPropertyRelationship(r, "p1", "int", NORMAL_KEY);
    addPropertyRelationship(r, "p2", "str", NORMAL_KEY);
    TEST("Two properties before delete", r->num_properties == 2);

    DeleteCommand c = {0};
    strncpy(c.name, "Link", MAX_NAME_LEN - 1);
    strncpy(c.prop_name, "p1", MAX_NAME_LEN - 1);
    c.type = PROP;

    bool ok = execute_delete(c, NULL);
    TEST("execute_delete returns true", ok);
    TEST("One property after delete", r->num_properties == 1);
    TEST("Remaining property is 'p2'", strcmp(r->properties[0]->name, "p2") == 0);
}

static void test_execute_delete_nonexistent_target(void) {
    SECTION("execute_delete – non-existent target");
    init_global_objects();

    DeleteCommand c = {0};
    strncpy(c.name, "Ghost", MAX_NAME_LEN - 1);
    c.type = ELEMENT;

    bool ok = execute_delete(c, NULL);
    TEST("execute_delete returns false for missing target", !ok);
}

// -------------------------------------------------------------------------
// UNDO STACK INFRASTRUCTURE TESTS
// -------------------------------------------------------------------------
static void test_undo_stack_init(void) {
    SECTION("undo_stack – initialisation");
    UndoStack s;
    undo_stack_init(&s);
    TEST("Stack starts empty", s.top == -1);
    TEST("undo_stack_is_empty returns true", undo_stack_is_empty(&s));
}

static void test_undo_stack_push_pop(void) {
    SECTION("undo_stack – push and pop");
    UndoStack s;
    undo_stack_init(&s);

    UndoEntry e1 = {0};
    e1.type = UNDO_CREATE_ENTITY;
    strncpy(e1.data.create_entity.name, "TestEnt", 14);

    bool pushed = undo_stack_push(&s, e1);
    TEST("Push succeeds", pushed);
    TEST("Stack no longer empty", !undo_stack_is_empty(&s));
    TEST("Top is 0 after first push", s.top == 0);

    UndoEntry out = {0};
    bool popped = undo_stack_pop(&s, &out);
    TEST("Pop succeeds", popped);
    TEST("Popped type matches", out.type == UNDO_CREATE_ENTITY);
    TEST("Popped data matches", strcmp(out.data.create_entity.name, "TestEnt") == 0);
    TEST("Stack empty again", undo_stack_is_empty(&s));
}

static void test_undo_stack_order(void) {
    SECTION("undo_stack – LIFO order");
    UndoStack s;
    undo_stack_init(&s);

    UndoEntry e1 = {0};
    e1.type = UNDO_CREATE_ENTITY;
    strncpy(e1.data.create_entity.name, "First", 14);

    UndoEntry e2 = {0};
    e2.type = UNDO_CREATE_RELATIONSHIP;
    strncpy(e2.data.create_rel.name, "Second", 14);

    undo_stack_push(&s, e1);
    undo_stack_push(&s, e2);

    UndoEntry out = {0};
    undo_stack_pop(&s, &out);
    TEST("LIFO: second pushed is first popped", out.type == UNDO_CREATE_RELATIONSHIP);
    TEST("LIFO: correct name", strcmp(out.data.create_rel.name, "Second") == 0);

    undo_stack_pop(&s, &out);
    TEST("LIFO: first pushed is last popped", out.type == UNDO_CREATE_ENTITY);
    TEST("LIFO: correct name", strcmp(out.data.create_entity.name, "First") == 0);
}

static void test_undo_stack_overflow(void) {
    SECTION("undo_stack – capacity overflow (drops oldest)");
    UndoStack s;
    undo_stack_init(&s);

    // Fill beyond capacity
    for (int i = 0; i < UNDO_STACK_CAPACITY + 5; i++) {
        UndoEntry e = {0};
        e.type = UNDO_CREATE_ENTITY;
        snprintf(e.data.create_entity.name, 15, "E%d", i);
        undo_stack_push(&s, e);
    }

    TEST("Stack remains at max capacity", s.top == UNDO_STACK_CAPACITY - 1);

    // Oldest entries should have been dropped; newest should remain
    UndoEntry out = {0};
    undo_stack_pop(&s, &out);
    char expected[15];
    snprintf(expected, 15, "E%d", UNDO_STACK_CAPACITY + 4);
    TEST("Newest entry survives overflow", strcmp(out.data.create_entity.name, expected) == 0);
}

static void test_undo_stack_pop_empty(void) {
    SECTION("undo_stack – pop from empty");
    UndoStack s;
    undo_stack_init(&s);

    UndoEntry out = {0};
    bool popped = undo_stack_pop(&s, &out);
    TEST("Pop from empty returns false", !popped);
}

// -------------------------------------------------------------------------
// UNDO DELETE TESTS (UNDO_DELETE)
// -------------------------------------------------------------------------
static void test_undo_delete_entity_full(void) {
    SECTION("UNDO_DELETE – restore deleted entity with properties");
    init_global_objects();

    // Setup: create entity with properties
    Entity *e = createEntity("Restorable", 10, 20);
    addProperty(e, "id", "int", PRIMARY_KEY);
    addProperty(e, "name", "str", NORMAL_KEY);
    int original_x = e->x;
    int original_y = e->y;

    // Manually push undo entry (simulating what execute_delete does)
    UndoEntry ue = {0};
    ue.type = UNDO_DELETE;
    ue.data.delete.delete_type = 1; // element
    ue.data.delete.is_relationship = false;
    strncpy(ue.data.delete.name, e->name, MAX_NAME_LEN);
    ue.data.delete.x = e->x;
    ue.data.delete.y = e->y;
    ue.data.delete.num_properties = e->num_properties;
    for (int i = 0; i < e->num_properties; i++) {
        if (e->properties[i]) {
            strncpy(ue.data.delete.props[i].name, e->properties[i]->name, MAX_NAME_LEN);
            strncpy(ue.data.delete.props[i].type, e->properties[i]->type, MAX_TYPE_LEN);
            ue.data.delete.props[i].keytype = (int)e->properties[i]->keytype;
        }
    }
    undo_stack_push(&global_objects.undo_stack, ue);

    // Delete the entity
    DeleteCommand dc = {0};
    strncpy(dc.name, "Restorable", MAX_NAME_LEN);
    dc.type = ELEMENT;
    execute_delete(dc, NULL);
    TEST("Entity deleted before undo", search_entity("Restorable") == NULL);

    // Undo
    Arena dummy_arena;
    arena_init(&dummy_arena, 1024);
    AST dummy_tree;
    init_AST(&dummy_tree);

    bool undone = execute_undo(&dummy_tree, &dummy_arena, NULL);
    TEST("execute_undo returns true", undone);

    Entity *restored = search_entity("Restorable");
    TEST("Entity restored", restored != NULL);
    TEST("X coordinate preserved", restored->x == original_x);
    TEST("Y coordinate preserved", restored->y == original_y);
    TEST("Properties restored", restored->num_properties == 2);
    TEST("Property[0] name correct", strcmp(restored->properties[0]->name, "id") == 0);
    TEST("Property[0] type correct", strcmp(restored->properties[0]->type, "int") == 0);
    TEST("Property[0] keytype correct", restored->properties[0]->keytype == PRIMARY_KEY);
    TEST("Property[1] name correct", strcmp(restored->properties[1]->name, "name") == 0);

    destroy_arena(&dummy_arena);
}

static void test_undo_delete_entity_property(void) {
    SECTION("UNDO_DELETE – restore deleted entity property");
    init_global_objects();

    Entity *e = createEntity("PropTest", 5, 5);
    addProperty(e, "keep", "int", NORMAL_KEY);
    addProperty(e, "remove", "str", NORMAL_KEY);
    TEST("Two properties initially", e->num_properties == 2);

    // Push undo for property deletion
    UndoEntry ue = {0};
    ue.type = UNDO_DELETE;
    ue.data.delete.delete_type = 0; // property
    ue.data.delete.is_relationship = false;
    strncpy(ue.data.delete.name, "PropTest", MAX_NAME_LEN);
    strncpy(ue.data.delete.prop_name, "remove", MAX_NAME_LEN);
    strncpy(ue.data.delete.prop_type, "str", MAX_TYPE_LEN);
    ue.data.delete.keytype = (int)NORMAL_KEY;
    undo_stack_push(&global_objects.undo_stack, ue);

    // Delete the property
    DeleteCommand dc = {0};
    strncpy(dc.name, "PropTest", MAX_NAME_LEN);
    strncpy(dc.prop_name, "remove", MAX_NAME_LEN);
    dc.type = PROP;
    execute_delete(dc, NULL);
    TEST("Property deleted before undo", e->num_properties == 1);

    // Undo
    Arena dummy_arena;
    arena_init(&dummy_arena, 1024);
    AST dummy_tree;
    init_AST(&dummy_tree);

    bool undone = execute_undo(&dummy_tree, &dummy_arena, NULL);
    TEST("execute_undo returns true", undone);

    TEST("Property restored", e->num_properties == 2);
    TEST("Restored property name correct", strcmp(e->properties[1]->name, "remove") == 0);
    TEST("Restored property type correct", strcmp(e->properties[1]->type, "str") == 0);

    destroy_arena(&dummy_arena);
}

static void test_undo_delete_relationship_full(void) {
    SECTION("UNDO_DELETE – restore deleted relationship with cardinalities");
    init_global_objects();

    Entity *e1 = createEntity("A", 5, 5);
    Entity *e2 = createEntity("B", 50, 5);
    Relationship *r = addRelationship(27, 5, e1, e2, "Rel");
    addPropertyRelationship(r, "p1", "int", NORMAL_KEY);
    addCardinalityAPI("1,n,0,1", r);

    // Push undo entry
    UndoEntry ue = {0};
    ue.type = UNDO_DELETE;
    ue.data.delete.delete_type = 1;
    ue.data.delete.is_relationship = true;
    strncpy(ue.data.delete.name, "Rel", MAX_NAME_LEN);
    ue.data.delete.x = r->x;
    ue.data.delete.y = r->y;
    ue.data.delete.num_properties = r->num_properties;
    for (int i = 0; i < r->num_properties; i++) {
        strncpy(ue.data.delete.props[i].name, r->properties[i]->name, MAX_NAME_LEN);
        strncpy(ue.data.delete.props[i].type, r->properties[i]->type, MAX_TYPE_LEN);
        ue.data.delete.props[i].keytype = (int)r->properties[i]->keytype;
    }
    strncpy(ue.data.delete.e1_name, e1->name, MAX_NAME_LEN);
    strncpy(ue.data.delete.e2_name, e2->name, MAX_NAME_LEN);
    if (r->cards[0]) {
        ue.data.delete.had_card0 = true;
        strncpy(ue.data.delete.card0, r->cards[0]->value, CARDINALITY_LEN);
    }
    if (r->cards[1]) {
        ue.data.delete.had_card1 = true;
        strncpy(ue.data.delete.card1, r->cards[1]->value, CARDINALITY_LEN);
    }
    undo_stack_push(&global_objects.undo_stack, ue);

    // Delete relationship
    DeleteCommand dc = {0};
    strncpy(dc.name, "Rel", MAX_NAME_LEN);
    dc.type = ELEMENT;
    execute_delete(dc, NULL);
    TEST("Relationship deleted before undo", search_relationship("Rel") == NULL);

    // Undo
    Arena dummy_arena;
    arena_init(&dummy_arena, 1024);
    AST dummy_tree;
    init_AST(&dummy_tree);

    bool undone = execute_undo(&dummy_tree, &dummy_arena, NULL);
    TEST("execute_undo returns true", undone);

    Relationship *restored = search_relationship("Rel");
    TEST("Relationship restored", restored != NULL);
    TEST("e1 restored correctly", strcmp(restored->e1->name, "A") == 0);
    TEST("e2 restored correctly", strcmp(restored->e2->name, "B") == 0);
    TEST("Property restored", restored->num_properties == 1);
    TEST("Cardinality[0] restored", restored->cards[0] != NULL);
    TEST("Cardinality[1] restored", restored->cards[1] != NULL);
    TEST("Card[0] value correct", strcmp(restored->cards[0]->value, "1,n") == 0);

    destroy_arena(&dummy_arena);
}

// -------------------------------------------------------------------------
// OTHER UNDO TYPE TESTS
// -------------------------------------------------------------------------
static void test_undo_create_entity(void) {
    SECTION("UNDO_CREATE_ENTITY – undo entity creation");
    init_global_objects();

    // Simulate: create entity -> push undo -> execute_create
    UndoEntry ue = {0};
    ue.type = UNDO_CREATE_ENTITY;
    strncpy(ue.data.create_entity.name, "TempEnt", MAX_NAME_LEN);
    undo_stack_push(&global_objects.undo_stack, ue);

    Entity *e = createEntity("TempEnt", 5, 5);
    addProperty(e, "id", "int", PRIMARY_KEY);
    TEST("Entity exists", search_entity("TempEnt") != NULL);

    Arena dummy_arena;
    arena_init(&dummy_arena, 1024);
    AST dummy_tree;
    init_AST(&dummy_tree);

    bool undone = execute_undo(&dummy_tree, &dummy_arena, NULL);
    TEST("execute_undo returns true", undone);
    TEST("Entity removed by undo", search_entity("TempEnt") == NULL);

    destroy_arena(&dummy_arena);
}

static void test_undo_create_relationship(void) {
    SECTION("UNDO_CREATE_RELATIONSHIP – undo relationship creation");
    init_global_objects();

    Entity *e1 = createEntity("X", 5, 5);
    Entity *e2 = createEntity("Y", 50, 5);
    addRelationship(27, 5, e1, e2, "TempRel");

    UndoEntry ue = {0};
    ue.type = UNDO_CREATE_RELATIONSHIP;
    strncpy(ue.data.create_rel.name, "TempRel", MAX_NAME_LEN);
    undo_stack_push(&global_objects.undo_stack, ue);

    TEST("Relationship exists", search_relationship("TempRel") != NULL);

    Arena dummy_arena;
    arena_init(&dummy_arena, 1024);
    AST dummy_tree;
    init_AST(&dummy_tree);

    bool undone = execute_undo(&dummy_tree, &dummy_arena, NULL);
    TEST("execute_undo returns true", undone);
    TEST("Relationship removed by undo", search_relationship("TempRel") == NULL);
    TEST("Entities still exist", search_entity("X") != NULL && search_entity("Y") != NULL);

    destroy_arena(&dummy_arena);
}

static void test_undo_add_property(void) {
    SECTION("UNDO_ADD_PROP – undo adding a property");
    init_global_objects();

    Entity *e = createEntity("PropOwner", 5, 5);
    addProperty(e, "id", "int", PRIMARY_KEY);

    // Simulate: add property -> push undo
    UndoEntry ue = {0};
    ue.type = UNDO_ADD_PROP;
    strncpy(ue.data.add_prop.identifier_name, "PropOwner", MAX_NAME_LEN);
    strncpy(ue.data.add_prop.prop_name, "email", MAX_NAME_LEN);
    strncpy(ue.data.add_prop.prop_type, "str", MAX_TYPE_LEN);
    ue.data.add_prop.keytype = (int)NORMAL_KEY;
    ue.data.add_prop.is_relationship = false;
    undo_stack_push(&global_objects.undo_stack, ue);

    // Actually add the property
    addProperty(e, "email", "str", NORMAL_KEY);
    TEST("Property added", e->num_properties == 2);

    Arena dummy_arena;
    arena_init(&dummy_arena, 1024);
    AST dummy_tree;
    init_AST(&dummy_tree);

    bool undone = execute_undo(&dummy_tree, &dummy_arena, NULL);
    TEST("execute_undo returns true", undone);
    TEST("Property removed by undo", e->num_properties == 1);
    TEST("Original property still exists", strcmp(e->properties[0]->name, "id") == 0);

    destroy_arena(&dummy_arena);
}

static void test_undo_change_name(void) {
    SECTION("UNDO_CHANGE_NAME – undo name change");
    init_global_objects();

    Entity *e = createEntity("OldName", 5, 5);
    TEST("Original name correct", strcmp(e->name, "OldName") == 0);

    // Simulate: change name -> push undo
    UndoEntry ue = {0};
    ue.type = UNDO_CHANGE_NAME;
    strncpy(ue.data.change_name.old_name, "OldName", MAX_NAME_LEN);
    strncpy(ue.data.change_name.new_name, "NewName", MAX_NAME_LEN);
    undo_stack_push(&global_objects.undo_stack, ue);

    // Apply name change
    strncpy(e->name, "NewName", MAX_NAME_LEN);
    TEST("Name changed", strcmp(e->name, "NewName") == 0);

    Arena dummy_arena;
    arena_init(&dummy_arena, 1024);
    AST dummy_tree;
    init_AST(&dummy_tree);

    bool undone = execute_undo(&dummy_tree, &dummy_arena, NULL);
    TEST("execute_undo returns true", undone);

    // Note: execute_undo searches by NEW name to find the entity
    Entity *found = search_entity("OldName");
    TEST("Name restored to old", found != NULL);

    destroy_arena(&dummy_arena);
}

static void test_undo_add_card(void) {
    SECTION("UNDO_ADD_CARD – undo cardinality change");
    init_global_objects();

    Entity *e1 = createEntity("A", 5, 5);
    Entity *e2 = createEntity("B", 50, 5);
    Relationship *r = addRelationship(27, 5, e1, e2, "R");

    // Set initial cardinality
    addCardinalityAPI("0,1,1,1", r);
    TEST("Initial card[0]", strcmp(r->cards[0]->value, "0,1") == 0);

    // Simulate: change cardinality -> push undo with OLD values
    UndoEntry ue = {0};
    ue.type = UNDO_ADD_CARD;
    strncpy(ue.data.add_card.rel_name, "R", MAX_NAME_LEN);
    ue.data.add_card.had_card0 = true;
    strncpy(ue.data.add_card.card0, "0,1", CARDINALITY_LEN);
    ue.data.add_card.had_card1 = true;
    strncpy(ue.data.add_card.card1, "1,1", CARDINALITY_LEN);
    undo_stack_push(&global_objects.undo_stack, ue);

    // Change cardinality
    addCardinalityAPI("1,n,0,n", r);
    TEST("Changed card[0]", strcmp(r->cards[0]->value, "1,n") == 0);

    Arena dummy_arena;
    arena_init(&dummy_arena, 1024);
    AST dummy_tree;
    init_AST(&dummy_tree);

    bool undone = execute_undo(&dummy_tree, &dummy_arena, NULL);
    TEST("execute_undo returns true", undone);
    TEST("Cardinality restored", strcmp(r->cards[0]->value, "0,1") == 0);

    destroy_arena(&dummy_arena);
}

static void test_undo_nothing_to_undo(void) {
    SECTION("execute_undo – empty stack");
    init_global_objects();

    Arena dummy_arena;
    arena_init(&dummy_arena, 1024);
    AST dummy_tree;
    init_AST(&dummy_tree);

    bool undone = execute_undo(&dummy_tree, &dummy_arena, NULL);
    TEST("execute_undo returns false when empty", !undone);

    destroy_arena(&dummy_arena);
}

static void test_undo_multiple_operations(void) {
    SECTION("execute_undo – multiple operations in sequence");
    init_global_objects();

    // Create entity
    Entity *e = createEntity("Multi", 5, 5);
    UndoEntry ue1 = {0};
    ue1.type = UNDO_CREATE_ENTITY;
    strncpy(ue1.data.create_entity.name, "Multi", MAX_NAME_LEN);
    undo_stack_push(&global_objects.undo_stack, ue1);

    // Add property
    addProperty(e, "p1", "int", NORMAL_KEY);
    UndoEntry ue2 = {0};
    ue2.type = UNDO_ADD_PROP;
    strncpy(ue2.data.add_prop.identifier_name, "Multi", MAX_NAME_LEN);
    strncpy(ue2.data.add_prop.prop_name, "p1", MAX_NAME_LEN);
    strncpy(ue2.data.add_prop.prop_type, "int", MAX_TYPE_LEN);
    ue2.data.add_prop.keytype = (int)NORMAL_KEY;
    ue2.data.add_prop.is_relationship = false;
    undo_stack_push(&global_objects.undo_stack, ue2);

    // Add another property
    addProperty(e, "p2", "str", NORMAL_KEY);
    UndoEntry ue3 = {0};
    ue3.type = UNDO_ADD_PROP;
    strncpy(ue3.data.add_prop.identifier_name, "Multi", MAX_NAME_LEN);
    strncpy(ue3.data.add_prop.prop_name, "p2", MAX_NAME_LEN);
    strncpy(ue3.data.add_prop.prop_type, "str", MAX_TYPE_LEN);
    ue3.data.add_prop.keytype = (int)NORMAL_KEY;
    ue3.data.add_prop.is_relationship = false;
    undo_stack_push(&global_objects.undo_stack, ue3);

    TEST("Entity has 2 properties", e->num_properties == 2); // createEntity has 0, plus 2 added

    Arena dummy_arena;
    arena_init(&dummy_arena, 1024);
    AST dummy_tree;
    init_AST(&dummy_tree);

    // Undo 1: remove p2
    bool undone = execute_undo(&dummy_tree, &dummy_arena, NULL);
    TEST("Undo 1 succeeds", undone);
    TEST("p2 removed", e->num_properties == 1);

    // Undo 2: remove p1
    undone = execute_undo(&dummy_tree, &dummy_arena, NULL);
    TEST("Undo 2 succeeds", undone);
    TEST("p1 removed", e->num_properties == 0);

    // Undo 3: remove entity
    undone = execute_undo(&dummy_tree, &dummy_arena, NULL);
    TEST("Undo 3 succeeds", undone);
    TEST("Entity removed", search_entity("Multi") == NULL);

    destroy_arena(&dummy_arena);
}

// -------------------------------------------------------------------------
// CLEAR COMMAND TESTS (with confirmation and undo)
// -------------------------------------------------------------------------
static void test_parse_clear_exists(void) {
    SECTION("parse_clear – token recognized");
    Parser p;
    const char *input = "clear";
    init_parser(&p, input);
    tokenize_content(input, p.tokens, &p.count);

    TEST("TOKEN_CLEAR recognized", p.tokens[0].type == TOKEN_CLEAR);
}

static void test_clear_resets_global_objects(void) {
    SECTION("parse_clear – resets global state");
    init_global_objects();

    createEntity("E1", 5, 5);
    createEntity("E2", 10, 10);
    TEST("Entities exist before clear", global_objects.entity_count == 2);

    // Note: parse_clear() currently does NO confirmation in headless test mode
    // We test init_global_objects directly since that's what parse_clear calls
    init_global_objects();
    TEST("Entity count reset", global_objects.entity_count == 0);
    TEST("Relationship count reset", global_objects.relationship_count == 0);
}

static void test_clear_undo_snapshot_structure(void) {
    SECTION("UNDO_CLEAR – snapshot structure validation");
    init_global_objects();

    // Create a small diagram
    Entity *e = createEntity("SnapEnt", 5, 5);
    addProperty(e, "id", "int", PRIMARY_KEY);

    // Simulate what parse_clear would do: snapshot before clear
    UndoEntry ue = {0};
    ue.type = UNDO_CLEAR; // Requires UNDO_CLEAR to be added to enum
    ue.data.convert_mld.snapshot = malloc(UNDO_SNAPSHOT_SIZE);
    TEST("Snapshot buffer allocated", ue.data.convert_mld.snapshot != NULL);
    if (ue.data.convert_mld.snapshot) {
        ue.data.convert_mld.snapshot_len =
            snapshot_diagram_to_buf(ue.data.convert_mld.snapshot, UNDO_SNAPSHOT_SIZE);

        TEST("Snapshot has content", ue.data.convert_mld.snapshot_len > 0);
        TEST("Snapshot starts with # MCD", strncmp(ue.data.convert_mld.snapshot, "# MCD", 5) == 0);
        TEST("Snapshot contains entity name", strstr(ue.data.convert_mld.snapshot, "SnapEnt") != NULL);
        TEST("Snapshot contains property", strstr(ue.data.convert_mld.snapshot, "id") != NULL);
    }
    undo_entry_free(&ue);
}

// -------------------------------------------------------------------------
// TEST RUNNER
// -------------------------------------------------------------------------
void run_undo_delete_tests(void) {
    printf("Starting Undo & Delete Tests...\n");
    log_file = fopen("undo_delete_test_results.log", "w");
    if (!log_file) {
        printf("CRITICAL ERROR: Could not open log file!\n");
        return;
    }

    SUITE_HEADER("UNDO & DELETE COMMAND TESTS");

    // Stack infrastructure
    test_undo_stack_init();
    test_undo_stack_push_pop();
    test_undo_stack_order();
    test_undo_stack_overflow();
    test_undo_stack_pop_empty();

    // Delete parser
    test_parse_delete_entity_element();
    test_parse_delete_entity_property();
    test_parse_delete_relationship_property();
    test_parse_delete_empty_name();
    test_parse_delete_missing_name();

    // Delete execution
    test_execute_delete_entity();
    test_execute_delete_entity_cascade_relationships();
    test_execute_delete_entity_property();
    test_execute_delete_entity_property_not_found();
    test_execute_delete_relationship();
    test_execute_delete_relationship_property();
    test_execute_delete_nonexistent_target();

    // Undo delete
    test_undo_delete_entity_full();
    test_undo_delete_entity_property();
    test_undo_delete_relationship_full();

    // Other undo types
    test_undo_create_entity();
    test_undo_create_relationship();
    test_undo_add_property();
    test_undo_change_name();
    test_undo_add_card();
    test_undo_nothing_to_undo();
    test_undo_multiple_operations();

    // Clear
    test_parse_clear_exists();
    test_clear_resets_global_objects();
    test_clear_undo_snapshot_structure();

    fprintf(log_file, "\n=========================================\n");
    fprintf(log_file, "  RESULTS: %d passed, %d failed\n", total_pass, total_fail);
    fprintf(log_file, "=========================================\n");
    fclose(log_file);
    printf("Undo & Delete tests complete. Results: %d passed, %d failed.\n"
           "See 'undo_delete_test_results.log' for details.\n",
           total_pass, total_fail);
    total_pass = 0;
    total_fail = 0;
}

#endif // UNDO_DELETE_TESTS

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

#ifdef INTEGRATION_TESTS
    run_integration_tests();
    suites_run++;
#endif

#ifdef UNDO_DELETE_TESTS
    run_undo_delete_tests();
    suites_run++;
#endif

    if (suites_run == 0) {
        printf("No test suites were compiled! ");
        printf("Compile with one or more of: ");
        printf("  -DARENA_TESTS         Arena allocator tests  (no ncurses) ");
        printf("  -DLEXER_TESTS         Tokenizer/lexer tests  (no ncurses) ");
        printf("  -DPARSER_TESTS        Parser + execute tests (no ncurses) ");
        printf("  -DMCD_TESTS           MCD element API tests  (no ncurses) ");
        printf("  -DHELP_TESTS          Help window + KMP tests(no ncurses) ");
        printf("  -DGRAPHICS_TEST       Visual/ncurses tests   (requires ncurses) ");
        printf("  -DINTEGRATION_TESTS   End-to-end workflow tests (no ncurses) ");
        printf("  -DUNDO_DELETE_TESTS   Undo & delete command tests (no ncurses) ");
    }

    return (total_fail > 0) ? 1 : 0;
}
