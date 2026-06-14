CC     = gcc
CFLAGS = -Wall -g -Wextra -Isrc -Isrc/DSA
LIBS   = -lncurses

# Source files
CORE_SRC = \
	src/MCD_elements.c \
	src/graphics.c \
	src/global_objects.c \
	src/DSA/astar.c \
	src/DSA/kmp.c \
	src/DSA/vec.c \
	src/DSA/pqueue.c \
	src/DSA/AST.c \
	src/Lexer/parse.c \
	src/Lexer/tokenize.c \
	src/utils/arena_allocator.c \
	src/command_processor.c \
	src/help_window.c

APP_SRC = src/main.c $(CORE_SRC)

# Object files: every .c in APP_SRC gets a .o in build/
OBJ_DIR = build
APP_OBJ = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(APP_SRC))

# === headless test source sets (no ncurses needed) ===
TEST_ARENA_SRC   = src/testing/tests.c src/utils/arena_allocator.c
TEST_LEXER_SRC   = src/testing/tests.c src/Lexer/tokenize.c
TEST_PARSER_SRC  = src/testing/tests.c $(CORE_SRC)
TEST_MCD_SRC     = src/testing/tests.c $(CORE_SRC)
TEST_HELP_SRC    = src/testing/tests.c $(CORE_SRC)

# === visual test set (ncurses required) ===
TEST_GRAPHICS_SRC = src/testing/tests.c $(CORE_SRC)

# === Targets ===
TARGET = main

all: $(TARGET)

# Link
$(TARGET): $(APP_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo "\n--- Built Main Application successfully ---"

# Compile each .c to a .o, auto-generating header dependency files (.d)
# -MMD -MP tells gcc to write a .d file listing every header the .c includes.
# Make reads those .d files and knows to rebuild the .o (and therefore the binary)
# whenever any included header changes, not just the .c itself.
$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Pull in the generated dependency files so header changes trigger rebuilds
-include $(APP_OBJ:.o=.d)

# Headless suites: no -lncurses
test_arena:
	$(CC) $(CFLAGS) -DARENA_TESTS -o test_arena $(TEST_ARENA_SRC)
	@echo "\n--- Built Arena Tests. Run: ./test_arena ---"
	./test_arena

test_lexer:
	$(CC) $(CFLAGS) -DLEXER_TESTS -o test_lexer $(TEST_LEXER_SRC)
	@echo "\n--- Built Lexer Tests. Run: ./test_lexer ---"
	./test_lexer

test_parser:
	$(CC) $(CFLAGS) -DPARSER_TESTS -o test_parser $(TEST_PARSER_SRC) $(LIBS)
	@echo "\n--- Built Parser Tests. Run: ./test_parser ---"
	./test_parser

test_mcd:
	$(CC) $(CFLAGS) -DMCD_TESTS -o test_mcd $(TEST_MCD_SRC) $(LIBS)
	@echo "\n--- Built MCD Element Tests. Run: ./test_mcd ---"
	./test_mcd

test_help:
	$(CC) $(CFLAGS) -DHELP_TESTS -o test_help $(TEST_HELP_SRC) $(LIBS)
	@echo "\n--- Built Help/KMP Tests. Run: ./test_help ---"
	./test_help

# visual suite: ncurses required, interactive
test_graphics:
	$(CC) $(CFLAGS) -DGRAPHICS_TEST -o test_graphics $(TEST_GRAPHICS_SRC) $(LIBS)
	@echo "\n--- Built Graphics Tests. Run: ./test_graphics ---"

test_all: test_arena test_lexer test_parser test_mcd test_help
	@echo "\n=== All headless test suites complete ==="
	@echo "Log files: arena_test_results.log   lexer_test_results.log"
	@echo "           parser_test_results.log   mcd_test_results.log"
	@echo "           help_test_results.log"

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET) test_arena test_lexer test_parser test_mcd test_help test_graphics \
	      *.log gmon.out
	@echo "Cleaned up all executables and generated files."

run: $(TARGET)
	./$(TARGET)
