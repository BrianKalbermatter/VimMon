#include "parser.h"
#include <stdio.h>
#include <string.h>

int paed_parse_line(const char *line, PAEDLine *out) {
    memset(out, 0, sizeof(*out));

    char buf[512];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *token = strtok(buf, " \t\n");
    if (!token || token[0] == '/' || token[0] == '#') return -1;

    strncpy(out->type, token, PAED_TYPE_MAX - 1);

    while ((token = strtok(NULL, " \t\n")) && out->param_count < PAED_MAX_PARAMS) {
        char *eq = strchr(token, '=');
        if (!eq) continue;
        *eq = '\0';
        strncpy(out->params[out->param_count].key, token,   PAED_KEY_MAX - 1);
        strncpy(out->params[out->param_count].val, eq + 1,  PAED_VAL_MAX - 1);
        out->param_count++;
    }

    return 0;
}

int paed_parse_file(const char *path, PAEDScene *out) {
    memset(out, 0, sizeof(*out));

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[512];
    while (fgets(line, sizeof(line), f) && out->line_count < PAED_MAX_LINES) {
        PAEDLine parsed;
        if (paed_parse_line(line, &parsed) == 0)
            out->lines[out->line_count++] = parsed;
    }

    fclose(f);
    return 0;
}

const char *paed_get_param(const PAEDLine *line, const char *key) {
    for (int i = 0; i < line->param_count; i++)
        if (strcmp(line->params[i].key, key) == 0)
            return line->params[i].val;
    return NULL;
}
