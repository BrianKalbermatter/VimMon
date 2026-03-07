CC = clang
CFLAGS = -Wall -Wextra -I src
LDFLAGS = -lSDL2 -lSDL2_ttf
# Solo archivos .c
SRC = src/main.c src/ui.c src/niveles.c src/progreso.c src/cJSON.c 
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
