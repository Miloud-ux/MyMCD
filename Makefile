CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lncurses

SRC = src/main.c src/MCD_elements.c
TARGET = main

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

