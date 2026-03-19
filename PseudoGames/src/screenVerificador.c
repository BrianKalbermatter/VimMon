#include "screenVerificador.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define INPUT_MAX 64

/* Dibuja un recuadro centrado con borde */
static void dibujar_panel(SDL_Renderer *r, int ancho, int alto,
                           int pad_x, int pad_y)
{
    SDL_Rect bg = { pad_x, pad_y, ancho - pad_x * 2, alto - pad_y * 2 };
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 10, 14, 22, 230);
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawColor(r, 60, 130, 255, 200);
    SDL_RenderDrawRect(r, &bg);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* Compara la respuesta del jugador con el esperado dentro de la tolerancia.
   Si el string no es numerico, hace comparacion exacta de texto. */
static int comparar(const char *respuesta, const char *esperado, const char *tolerancia)
{
    char *endR, *endE;
    double vR = strtod(respuesta, &endR);
    double vE = strtod(esperado,  &endE);

    /* si ambos son numericos, compara con tolerancia */
    if (endR != respuesta && endE != esperado) {
        double tol = strtod(tolerancia, NULL);
        return fabs(vR - vE) <= tol;
    }

    /* sino, comparacion de texto (sin importar mayusculas) */
    return strcasecmp(respuesta, esperado) == 0;
}

int screenVerificador(SDL_Renderer *renderer, TTF_Font *fuente,
                      int ancho, int alto, Nivel *n)
{
    if (!n || n->num_casos == 0) return 1;  /* sin casos = pasa directamente */

    int caso_actual = 0;
    char input[INPUT_MAX] = {0};
    int input_len = 0;
    char msg_error[128] = {0};
    int mostrar_error = 0;

    SDL_StartTextInput();

    SDL_Color c_titulo  = {180, 200, 255, 255};
    SDL_Color c_datos   = {160, 160, 180, 255};
    SDL_Color c_pregunta= {220, 230, 255, 255};
    SDL_Color c_input   = {255, 255, 255, 255};
    SDL_Color c_error   = {255,  80,  80, 255};
    SDL_Color c_hint    = {120, 120, 140, 255};

    int pad_x = 80, pad_y = 60;
    int ix = pad_x + 30;  /* x base del texto dentro del panel */

    while (1) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                SDL_StopTextInput();
                return 0;
            }

            if (ev.type == SDL_KEYDOWN) {
                switch (ev.key.keysym.sym) {

                    case SDLK_ESCAPE:
                        SDL_StopTextInput();
                        return 0;

                    case SDLK_BACKSPACE:
                        if (input_len > 0) {
                            input[--input_len] = '\0';
                            mostrar_error = 0;
                        }
                        break;

                    case SDLK_RETURN: {
                        if (input_len == 0) break;

                        CasoPrueba *cp = &n->casos[caso_actual];
                        if (comparar(input, cp->esperado, cp->tolerancia)) {
                            /* caso superado */
                            caso_actual++;
                            input[0] = '\0';
                            input_len = 0;
                            mostrar_error = 0;

                            if (caso_actual >= n->num_casos) {
                                /* TODOS los casos pasaron */
                                SDL_StopTextInput();
                                return 1;
                            }
                        } else {
                            /* respuesta incorrecta */
                            snprintf(msg_error, sizeof(msg_error),
                                     "Incorrecto. Esperado: %s", cp->esperado);
                            mostrar_error = 1;
                        }
                        break;
                    }

                    default: break;
                }
            }

            if (ev.type == SDL_TEXTINPUT) {
                int add = (int)strlen(ev.text.text);
                if (input_len + add < INPUT_MAX - 1) {
                    strcat(input, ev.text.text);
                    input_len += add;
                    mostrar_error = 0;
                }
            }
        }

        /* ── render ── */
        SDL_SetRenderDrawColor(renderer, 8, 10, 18, 255);
        SDL_RenderClear(renderer);

        dibujar_panel(renderer, ancho, alto, pad_x, pad_y);

        CasoPrueba *cp = &n->casos[caso_actual];

        /* titulo */
        char str_titulo[160];
        snprintf(str_titulo, sizeof(str_titulo),
                 "Verificacion — %s", n->titulo);
        dibujadoTextoColor(renderer, fuente, str_titulo, ix, pad_y + 24, c_titulo);

        /* progreso de casos */
        char str_caso[64];
        snprintf(str_caso, sizeof(str_caso),
                 "Caso %d de %d", caso_actual + 1, n->num_casos);
        dibujadoTextoColor(renderer, fuente, str_caso, ix, pad_y + 54, c_hint);

        /* separador */
        SDL_SetRenderDrawColor(renderer, 60, 80, 120, 255);
        SDL_RenderDrawLine(renderer, pad_x + 20, pad_y + 76,
                           ancho - pad_x - 20, pad_y + 76);

        /* datos del caso */
        char str_datos[300];
        snprintf(str_datos, sizeof(str_datos), "Datos:    %s", cp->datos);
        dibujadoTextoColor(renderer, fuente, str_datos, ix, pad_y + 92, c_datos);

        /* pregunta */
        dibujadoTextoColor(renderer, fuente, cp->pregunta, ix, pad_y + 130, c_pregunta);

        /* caja de input */
        int box_y = pad_y + 170;
        SDL_Rect box = { ix, box_y, ancho - pad_x * 2 - 60, 36 };
        SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
        SDL_RenderFillRect(renderer, &box);
        SDL_SetRenderDrawColor(renderer, 80, 140, 255, 255);
        SDL_RenderDrawRect(renderer, &box);

        /* texto del input + cursor parpadeante */
        char display[INPUT_MAX + 2];
        Uint32 t = SDL_GetTicks();
        if ((t / 500) % 2 == 0)
            snprintf(display, sizeof(display), "%s|", input);
        else
            snprintf(display, sizeof(display), "%s ", input);
        dibujadoTextoColor(renderer, fuente, display, ix + 8, box_y + 8, c_input);

        /* mensaje de error o de ok parcial */
        if (mostrar_error) {
            dibujadoTextoColor(renderer, fuente, msg_error,
                               ix, box_y + 48, c_error);
        }

        /* atajos */
        dibujadoTextoColor(renderer, fuente, "[ENTER] Confirmar    [ESC] Volver",
                           ix, alto - pad_y - 30, c_hint);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}
