CC = clang
CFLAGS = -Wall -Wextra -I src
LDFLAGS = -lncurses
SRC = src/main.c src/ui.c
# src/ui.c src/niveles.c src/progreso.c src/editor.c src/cJSON.c
OBJ = $(SRC:.c=.o)
TARGET = aed

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS) 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJ) $(TARGET) 

.PHONY: all clean
