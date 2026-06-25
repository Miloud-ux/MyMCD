# ============================================================================
# MyMCD – MCD Designer Build System
# Cross-platform: Linux (install target) & Windows (run target only)
# ============================================================================

CC     = gcc
CFLAGS = -Wall -g -Wextra -Isrc -Isrc/DSA
LIBS   = -lncurses

# -------------------------------------------------------------------------
# Detect OS for conditional rules
# -------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    DETECTED_OS := $(shell uname -s)
endif

# -------------------------------------------------------------------------
# Installation directories (Linux/Unix only)
# -------------------------------------------------------------------------
PREFIX  ?= /usr/local
BINDIR   = $(PREFIX)/bin

# -------------------------------------------------------------------------
# Source files
# -------------------------------------------------------------------------
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
	src/utils/save.c \
	src/utils/sql.c \
	src/utils/menu.c \
	src/command_processor.c \
	src/help_window.c

APP_SRC = src/main.c $(CORE_SRC)

# -------------------------------------------------------------------------
# Object files
# -------------------------------------------------------------------------
OBJ_DIR = build
APP_OBJ = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(APP_SRC))

# -------------------------------------------------------------------------
# Test source sets
# -------------------------------------------------------------------------
TEST_ARENA_SRC       = src/testing/tests.c src/utils/arena_allocator.c
TEST_LEXER_SRC       = src/testing/tests.c src/Lexer/tokenize.c
TEST_PARSER_SRC      = src/testing/tests.c $(CORE_SRC)
TEST_MCD_SRC         = src/testing/tests.c $(CORE_SRC)
TEST_HELP_SRC        = src/testing/tests.c $(CORE_SRC)
TEST_INTEGRATION_SRC = src/testing/tests.c $(CORE_SRC)
TEST_GRAPHICS_SRC    = src/testing/tests.c $(CORE_SRC)

# -------------------------------------------------------------------------
# Targets
# -------------------------------------------------------------------------
TARGET = MyMCD

# -------------------------------------------------------------------------
# Phony targets
# -------------------------------------------------------------------------
.PHONY: all clean run install uninstall \
        test_all test_arena test_lexer test_parser test_mcd test_help \
        test_integration test_graphics

# =========================================================================
# BUILD RULES
# =========================================================================

all: $(TARGET)

# Link final application
$(TARGET): $(APP_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo ""
	@echo "=== Built MyMCD successfully ==="
	@echo "Run with: ./MyMCD   (Linux)   or   MyMCD.exe   (Windows)"

# Compile .c to .o with auto-generated dependency files
$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(APP_OBJ:.o=.d)

# =========================================================================
# INSTALL / UNINSTALL (Linux/Unix only)
# =========================================================================

install: $(TARGET)
ifeq ($(DETECTED_OS),Windows)
	@echo "ERROR: 'make install' is not supported on Windows."
	@echo "On Windows, use 'make run' to build and run locally."
	@exit 1
else
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/MyMCD
	@echo ""
	@echo "=== Installed MyMCD to $(DESTDIR)$(BINDIR)/MyMCD ==="
	@echo "Run from anywhere with: MyMCD"
endif

uninstall:
ifeq ($(DETECTED_OS),Windows)
	@echo "ERROR: 'make uninstall' is not supported on Windows."
	@exit 1
else
	rm -f $(DESTDIR)$(BINDIR)/MyMCD
	@echo "Uninstalled MyMCD from $(DESTDIR)$(BINDIR)"
endif

# =========================================================================
# RUN (works on all platforms – builds then executes locally)
# =========================================================================

run: $(TARGET)
	./$(TARGET)

# =========================================================================
# TEST SUITES
# =========================================================================

test_arena:
	$(CC) $(CFLAGS) -DARENA_TESTS -o test_arena $(TEST_ARENA_SRC)
	@echo "--- Built Arena Tests ---"
	./test_arena

test_lexer:
	$(CC) $(CFLAGS) -DLEXER_TESTS -o test_lexer $(TEST_LEXER_SRC)
	@echo "--- Built Lexer Tests ---"
	./test_lexer

test_parser:
	$(CC) $(CFLAGS) -DPARSER_TESTS -o test_parser $(TEST_PARSER_SRC) $(LIBS)
	@echo "--- Built Parser Tests ---"
	./test_parser

test_mcd:
	$(CC) $(CFLAGS) -DMCD_TESTS -o test_mcd $(TEST_MCD_SRC) $(LIBS)
	@echo "--- Built MCD Element Tests ---"
	./test_mcd

test_help:
	$(CC) $(CFLAGS) -DHELP_TESTS -o test_help $(TEST_HELP_SRC) $(LIBS)
	@echo "--- Built Help/KMP Tests ---"
	./test_help

test_integration:
	$(CC) $(CFLAGS) -DINTEGRATION_TESTS -o test_integration $(TEST_INTEGRATION_SRC) $(LIBS)
	@echo "--- Built Integration Tests ---"
	./test_integration

test_graphics:
	$(CC) $(CFLAGS) -DGRAPHICS_TEST -o test_graphics $(TEST_GRAPHICS_SRC) $(LIBS)
	@echo "--- Built Graphics Tests (interactive) ---"

test_all: test_arena test_lexer test_parser test_mcd test_help test_integration
	@echo ""
	@echo "=== All headless test suites complete ==="
	@echo "Log files:"
	@echo "  arena_test_results.log"
	@echo "  lexer_test_results.log"
	@echo "  parser_test_results.log"
	@echo "  mcd_test_results.log"
	@echo "  help_test_results.log"
	@echo "  integration_test_results.log"

# =========================================================================
# CLEAN
# =========================================================================

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET) \
	      test_arena test_lexer test_parser test_mcd test_help \
	      test_integration test_graphics \
	      *.log gmon.out
	@echo "Cleaned all build artifacts."
