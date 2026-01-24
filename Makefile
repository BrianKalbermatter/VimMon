# Makefile simple
# Uso: make NombreArchivo (sin extensión)

CC = clang
CFLAGS = -Wall -Wextra
LIBS = -lraylib -lGL -lm -lpthread -ldl
SRC_DIR = src
BIN_DIR = bin

# Regla genérica: make nombre → compila src/nombre.c → bin/nombre
%:
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(SRC_DIR)/$@.c $(LIBS) -o $(BIN_DIR)/$@

clean:
	rm -f $(BIN_DIR)/*
