CC     = clang

# Flags de SDL2 vía sdl2-config (no hay pkg-config en este equipo).
SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS   = $(shell sdl2-config --libs)

CFLAGS = -Wall -Wextra -I. $(SDL_CFLAGS)
LIBS   = -lcurl $(SDL_LIBS)

BUILD  = build
TARGET = $(BUILD)/vimmon

SRCS_BUS = bus/bus.c # bus de eventos
SRCS_AI = plugins/ai/ai.c # plugin de IA (Ollama)
SRCS_IDE = plugins/ide/ide.c plugins/ide/parser.c plugins/ide/interpreter.c # plugin IDE/PAED
SRCS_MONITOR = plugins/monitor/monitor.c # plugin monitor
SRCS_RENDERER = plugins/renderer/renderer.c plugins/renderer/sdl_fb.c # plugin renderer + backend
SRCS_ENGINE = engine/engine.c # motor 2D de entidades
SRCS_GAME = game/game.c # el juego del usuario
SRCS_INPUT = plugins/input/input.c # plugin de teclado + mouse

SRCS = main.c $(SRCS_BUS) cjson/cJSON.c $(SRCS_AI) $(SRCS_IDE) $(SRCS_MONITOR) $(SRCS_RENDERER) $(SRCS_ENGINE) $(SRCS_GAME) $(SRCS_INPUT)

$(TARGET): $(SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(SRCS) $(LIBS) -o $(TARGET)

# Ejemplo del motor: build/hello_entity (autocontenido, sin el bus).
example: $(BUILD)/hello_entity

$(BUILD)/hello_entity: examples/hello_entity.c $(SRCS_ENGINE) plugins/renderer/sdl_fb.c | $(BUILD)
	$(CC) $(CFLAGS) $^ $(SDL_LIBS) -o $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)

.PHONY: clean example
