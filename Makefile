CC = gcc
CFLAGS = -Wall -Wextra -Isrc -Isrc/DSA
LIBS = -lncurses

SRC = src/main.c \
      src/MCD_elements.c \
      src/graphics.c \
      src/global_objects.c \
      src/DSA/astar.c \
	  src/parse_commands.c \
      src/DSA/pqueue.c

TARGET = main

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)


