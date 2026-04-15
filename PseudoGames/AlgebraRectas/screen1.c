#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "screen1.h"

// Escala y origen del plano cartesiano en pixels
// PAED no sabe nada de esto — solo C convierte
#define ESCALA  100
#define CX      400
#define CY      400
#define ANCHO   800
#define ALTO    800

// Conversion coordenada matematica → pixel
static float px(float x) { return CX + x * ESCALA; }
static float py(float y) { return CY - y * ESCALA; }

void
cargar_paed(const char* archivo, Comando* cmds, int* n)
{
    char cmd_shell[512];
    snprintf(cmd_shell, sizeof(cmd_shell),
        "cd ../Frankly && ./paed '../AlgebraRectas/%s' 2>/dev/null", archivo);

    FILE* pipe = popen(cmd_shell, "r");
    if (!pipe) {
        fprintf(stderr, "Error: no se pudo ejecutar paed\n");
        return;
    }

    char linea[256];
    *n = 0;

    while (fgets(linea, sizeof(linea), pipe) && *n < MAX_COMANDOS) {
        char  tipo[16];
        float a = 0, b = 0, c = 0, d = 0;
        int count = sscanf(linea, "%15s %f %f %f %f", tipo, &a, &b, &c, &d);
        if (count >= 1) {
            strncpy(cmds[*n].tipo, tipo, sizeof(cmds[*n].tipo) - 1);
            cmds[*n].tipo[sizeof(cmds[*n].tipo) - 1] = '\0';
            cmds[*n].x1 = a;
            cmds[*n].y1 = b;
            cmds[*n].x2 = c;
            cmds[*n].y2 = d;
            (*n)++;
        }
    }

    pclose(pipe);
}

void
dibujar_comandos(SDL_Renderer* renderer, Comando* cmds, int n)
{
    char buf[16];

    for (int i = 0; i < n; i++) {
        char* tipo = cmds[i].tipo;

        if (strcmp(tipo, "EJE_X") == 0) {
            // Eje X: linea horizontal de borde a borde, a altura y=0
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderLine(renderer, 0, py(0), ANCHO, py(0));

        } else if (strcmp(tipo, "EJE_Y") == 0) {
            // Eje Y: linea vertical de borde a borde, en x=0
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderLine(renderer, px(0), 0, px(0), ALTO);

        } else if (strcmp(tipo, "TICK_X") == 0) {
            // Tick vertical sobre el eje X en la coordenada x=x1
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderLine(renderer, px(cmds[i].x1), py(0) - 5,
                                     px(cmds[i].x1), py(0) + 5);

        } else if (strcmp(tipo, "TICK_Y") == 0) {
            // Tick horizontal sobre el eje Y en la coordenada y=x1
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderLine(renderer, px(0) - 5, py(cmds[i].x1),
                                     px(0) + 5, py(cmds[i].x1));

        } else if (strcmp(tipo, "LABEL_X") == 0) {
            // Numero bajo el tick del eje X
            snprintf(buf, sizeof(buf), "%d", (int)cmds[i].y1);
            SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
            SDL_RenderDebugText(renderer, px(cmds[i].x1) - 3, py(0) + 8, buf);

        } else if (strcmp(tipo, "LABEL_Y") == 0) {
            // Numero a la izquierda del tick del eje Y
            snprintf(buf, sizeof(buf), "%d", (int)cmds[i].y1);
            SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
            SDL_RenderDebugText(renderer, px(0) - 22, py(cmds[i].x1) - 4, buf);

        } else if (strcmp(tipo, "LABEL_O") == 0) {
            // Etiqueta del origen
            SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
            SDL_RenderDebugText(renderer, px(0) + 5, py(0) + 5, "0");

        } else if (strcmp(tipo, "LINEA") == 0) {
            // Recta: coordenadas matematicas → pixels
            SDL_SetRenderDrawColor(renderer, 220, 50, 50, 255);
            SDL_RenderLine(renderer,
                px(cmds[i].x1), py(cmds[i].y1),
                px(cmds[i].x2), py(cmds[i].y2));
        }
    }
}
