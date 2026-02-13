# ============================================================
# FASE 4 - Ejercicio 4.7: Makefile para todos los proyectos
# ============================================================
# USO:
#   make           → compilar todo
#   make debug     → compilar con simbolos de debug
#   make release   → compilar optimizado
#   make clean     → borrar binarios
#   make micat     → compilar solo micat
#   make install   → instalar binarios en /usr/local/bin
# ============================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11
DEBUG_FLAGS = -g -O0 -DDEBUG
RELEASE_FLAGS = -O2 -DNDEBUG

# Fuentes y binarios
SRC_MICAT = ejercicio_4.1_micat.c
SRC_LISTA = ejercicio_4.2_lista_enlazada.c
SRC_LAUNCHER = ejercicio_4.3_launcher.c
SRC_SHELL = ejercicio_4.4_minishell.c
SRC_SERVER = ejercicio_4.5_servidor_tcp.c
SRC_SYSMON = ejercicio_4.6_sysmon.c

BIN_MICAT = micat
BIN_LISTA = lista
BIN_LAUNCHER = launcher
BIN_SHELL = minishell
BIN_SERVER = server
BIN_SYSMON = sysmon

ALL_BINS = $(BIN_MICAT) $(BIN_LISTA) $(BIN_LAUNCHER) $(BIN_SHELL) $(BIN_SERVER) $(BIN_SYSMON)

# Target por defecto: compilar todo
all: $(ALL_BINS)
	@echo ""
	@echo "=== Compilacion completa ==="
	@echo "Binarios generados:"
	@ls -la $(ALL_BINS)

# Targets individuales
$(BIN_MICAT): $(SRC_MICAT)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_LISTA): $(SRC_LISTA)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_LAUNCHER): $(SRC_LAUNCHER)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_SHELL): $(SRC_SHELL)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_SERVER): $(SRC_SERVER)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_SYSMON): $(SRC_SYSMON)
	$(CC) $(CFLAGS) -o $@ $<

# Compilar con debug
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean all
	@echo ""
	@echo "=== Compilado en modo DEBUG ==="

# Compilar optimizado
release: CFLAGS += $(RELEASE_FLAGS)
release: clean all
	@echo ""
	@echo "=== Compilado en modo RELEASE ==="

# Instalar en /usr/local/bin
install: all
	@echo "Instalando en /usr/local/bin..."
	install -m 755 $(ALL_BINS) /usr/local/bin/
	@echo "Instalado!"

# Desinstalar
uninstall:
	@echo "Desinstalando de /usr/local/bin..."
	cd /usr/local/bin && rm -f $(ALL_BINS)
	@echo "Desinstalado!"

# Limpiar binarios
clean:
	rm -f $(ALL_BINS)
	rm -f *.o
	@echo "Limpio!"

# Verificar con valgrind (solo lista enlazada)
valgrind: $(BIN_LISTA)
	valgrind --leak-check=full --show-leak-kinds=all ./$(BIN_LISTA)

.PHONY: all debug release clean install uninstall valgrind
