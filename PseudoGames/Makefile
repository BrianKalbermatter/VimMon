CC = clang
CFLAGS = -Wall -Wextra -I src
LDFLAGS = -lncurses
SRC = src/main.c src/ui.c src/niveles.c src/progreso.c src/editor.c src/cJSON.c
TARGET = aed

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
