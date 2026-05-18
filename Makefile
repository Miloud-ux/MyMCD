CC = gcc
CFLAGS = -Wall -pg -g -Wextra -Isrc -Isrc/DSA
LIBS = -lncurses

SRC = src/main.c \
      src/MCD_elements.c \
      src/graphics.c \
      src/global_objects.c \
      src/DSA/astar.c \
      src/Lexer/parse.c \
      src/Lexer/tokenize.c \
	  src/command_processor.c \
	  src/help_window.c \
      src/DSA/pqueue.c

TARGET = main

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)


