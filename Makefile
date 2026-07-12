CC     = clang
CFLAGS = -Wall -Wextra -I.
LIBS   = -lcurl

BUILD  = build
TARGET = $(BUILD)/vimmon

SRCS = main.c \
       bus/bus.c \
       cjson/cJSON.c \
       plugins/ai/ai.c \
       plugins/ide/ide.c \
       plugins/ide/parser.c \
       plugins/ide/interpreter.c \
       plugins/monitor/monitor.c \
       plugins/renderer/renderer.c

$(TARGET): $(SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(SRCS) $(LIBS) -o $(TARGET)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)

.PHONY: clean
