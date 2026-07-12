#ifndef VIMMON_PARSER_H
#define VIMMON_PARSER_H

#define PAED_TYPE_MAX    32
#define PAED_KEY_MAX     32
#define PAED_VAL_MAX    128
#define PAED_MAX_PARAMS  16
#define PAED_MAX_LINES  256

typedef struct {
    char key[PAED_KEY_MAX];
    char val[PAED_VAL_MAX];
} PAEDParam;

typedef struct {
    char      type[PAED_TYPE_MAX];
    PAEDParam params[PAED_MAX_PARAMS];
    int       param_count;
} PAEDLine;

typedef struct {
    PAEDLine lines[PAED_MAX_LINES];
    int      line_count;
} PAEDScene;

int         paed_parse_file(const char *path, PAEDScene *out);
int         paed_parse_line(const char *line, PAEDLine *out);
const char *paed_get_param (const PAEDLine *line, const char *key);

#endif // VIMMON_PARSER_H
