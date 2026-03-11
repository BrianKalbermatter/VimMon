# ── Linux (clang) ──────────────────────────────────────────────────────────
CC     = clang
CFLAGS = -Wall -Wextra -I src
LDFLAGS = -lSDL2 -lSDL2_ttf -lm

BUILDDIR = build

SRC = src/main.c src/ui.c src/niveles.c src/progreso.c src/cJSON.c \
      src/screenDOC.c src/screenMenu.c src/screenEditorLvl.c \
      src/screenLvLs.c src/screenEditorFree.c src/editorText.c \
      src/screenSoluction.c src/screenPomodoro.c src/pomodoro_bg.c \
      src/screenConfig.c src/screenFeedback.c

OBJ     = $(addprefix $(BUILDDIR)/, $(notdir $(SRC:.c=.o)))
TARGET  = aed

all: $(BUILDDIR) $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(BUILDDIR)/%.o: src/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# ── Windows cross-compile (mingw-w64) ──────────────────────────────────────
WIN_CC     = x86_64-w64-mingw32-gcc
WIN_LIBS   = win-libs
WIN_CFLAGS = -Wall -Wextra -I src -I $(WIN_LIBS)/include
WIN_LDFLAGS = -L $(WIN_LIBS)/lib \
              -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lm \
              -mwindows

WIN_OBJ    = $(addprefix $(BUILDDIR)/, $(notdir $(SRC:.c=.win.o)))
WIN_TARGET = PseudoGames.exe

windows: $(BUILDDIR) $(WIN_TARGET)

$(WIN_TARGET): $(WIN_OBJ)
	$(WIN_CC) $(WIN_OBJ) -o $@ $(WIN_LDFLAGS)

$(BUILDDIR)/%.win.o: src/%.c | $(BUILDDIR)
	$(WIN_CC) $(WIN_CFLAGS) -c $< -o $@

# ── Limpieza ────────────────────────────────────────────────────────────────
clean:
	rm -rf $(BUILDDIR) $(TARGET) $(WIN_TARGET)

.PHONY: all windows clean
