CC     = clang
CFLAGS = -Wall -Wextra -I.
LIBS   = -lcurl

BUILD  = build
TARGET = $(BUILD)/vimmon

SRCS_BUS = bus/bus.c # bus de eventos
SRCS_AI = plugins/ai/ai.c # plugin de IA (Ollama)
SRCS_IDE = plugins/ide/ide.c plugins/ide/parser.c plugins/ide/interpreter.c # plugin IDE/PAED
SRCS_MONITOR = plugins/monitor/monitor.c # plugin monitor
SRCS_RENDERER = plugins/renderer/renderer.c # plugin renderer

SRCS = main.c $(SRCS_BUS) cjson/cJSON.c $(SRCS_AI) $(SRCS_IDE) $(SRCS_MONITOR) $(SRCS_RENDERER)

$(TARGET): $(SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(SRCS) $(LIBS) -o $(TARGET)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)

.PHONY: clean
