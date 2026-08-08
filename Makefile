CC     = clang

# Flags de SDL2 vía sdl2-config (no hay pkg-config en este equipo).
SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS   = $(shell sdl2-config --libs)

# -MMD -MP hacen que clang, al compilar cada .c, escriba al lado un .d con la
# lista de headers que ese .c terminó incluyendo (incluso los indirectos).
# Esos .d se leen más abajo con -include, y así make sabe que tocar parser.h
# obliga a recompilar parser.c, ide.c y main.c. Sin esto, make solo mira los
# .c y un cambio en un header pasa desapercibido.
#   -MMD  genera el .d y solo lista headers propios (ignora los del sistema)
#   -MP   agrega un target vacío por header, para que borrar un header
#         no rompa el build con "No rule to make target"
CFLAGS = -Wall -Wextra -I. $(SDL_CFLAGS) -MMD -MP
LIBS   = -lcurl -lm $(SDL_LIBS)

BUILD  = build
OBJDIR = $(BUILD)/obj
TARGET = $(BUILD)/vimmon

SRCS_BUS = bus/bus.c # bus de eventos
SRCS_AI = plugins/ai/ai.c plugins/ai/provider.c # plugin de IA + selector de proveedor
SRCS_IDE = plugins/ide/ide.c plugins/ide/parser.c plugins/ide/interpreter.c plugins/ide/scene_view.c # plugin IDE/PAED
SRCS_MONITOR = plugins/monitor/monitor.c # plugin monitor
SRCS_RENDERER = plugins/renderer/renderer.c plugins/renderer/sdl_fb.c # plugin renderer + backend
SRCS_ENGINE = engine/engine.c # motor 2D de entidades
SRCS_GAME = game/game.c # el juego del usuario
SRCS_INPUT = plugins/input/input.c # plugin de teclado + mouse

SRCS = main.c $(SRCS_BUS) cjson/cJSON.c $(SRCS_AI) $(SRCS_IDE) $(SRCS_MONITOR) $(SRCS_RENDERER) $(SRCS_ENGINE) $(SRCS_GAME) $(SRCS_INPUT)

# plugins/ai/ai.c -> build/obj/plugins/ai/ai.o (y .d al lado)
OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)
DEPS = $(OBJS:.o=.d)

all: $(TARGET)

# Ahora se linkean objetos, no se recompila todo el proyecto de cero.
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) $(LIBS) -o $@

# Regla única: cualquier .c del proyecto se compila a su .o espejado en OBJDIR.
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Ejemplo del motor: build/hello_entity (autocontenido, sin el bus).
EXAMPLE_SRCS = examples/hello_entity.c $(SRCS_ENGINE) plugins/renderer/sdl_fb.c
EXAMPLE_OBJS = $(EXAMPLE_SRCS:%.c=$(OBJDIR)/%.o)
DEPS += $(EXAMPLE_OBJS:.o=.d)

example: $(BUILD)/hello_entity

$(BUILD)/hello_entity: $(EXAMPLE_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(EXAMPLE_OBJS) $(SDL_LIBS) -o $@

clean:
	rm -rf $(BUILD)

# Los .d no existen en el primer build: el '-' evita que make se queje.
-include $(DEPS)

.PHONY: all clean example
