CC      = gcc
TARGET  = rectas
SRCS    = main.c screen1.c
CFLAGS  = -Wall -Wextra $(shell pkg-config --cflags sdl3)
LIBS    = $(shell pkg-config --libs sdl3)

$(TARGET): $(SRCS) screen1.h
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: clean
