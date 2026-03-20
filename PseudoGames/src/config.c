#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
#include "config.h"

#define CONFIG_PATH "saves/config.json"

static int s_fullscreen = 0;  /* indice: 0=Si, 1=No  (orden del array opciones) */
static int s_lluvia     = 1;
static int s_brillo     = 1;

void
config_cargar(void)
{
    FILE *f = fopen(CONFIG_PATH, "r");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return;

    cJSON *v;
    if ((v = cJSON_GetObjectItem(root, "fullscreen")) && cJSON_IsNumber(v))
        s_fullscreen = v->valueint;
    if ((v = cJSON_GetObjectItem(root, "lluvia")) && cJSON_IsNumber(v))
        s_lluvia = v->valueint;
    if ((v = cJSON_GetObjectItem(root, "brillo")) && cJSON_IsNumber(v))
        s_brillo = v->valueint;

    cJSON_Delete(root);
}

void
config_guardar(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "fullscreen", s_fullscreen);
    cJSON_AddNumberToObject(root, "lluvia",     s_lluvia);
    cJSON_AddNumberToObject(root, "brillo",     s_brillo);

    char *str = cJSON_Print(root);
    cJSON_Delete(root);
    if (!str) return;

    FILE *f = fopen(CONFIG_PATH, "w");
    if (f) { fputs(str, f); fclose(f); }
    free(str);
}

int config_get_fullscreen(void) { return s_fullscreen; }
int config_get_lluvia(void)     { return s_lluvia; }
int config_get_brillo(void)     { return s_brillo; }

void config_set_fullscreen(int v) { s_fullscreen = v; }
void config_set_lluvia(int v)     { s_lluvia     = v; }
void config_set_brillo(int v)     { s_brillo     = v; }
