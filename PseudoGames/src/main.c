#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include "niveles.h"
#include "progreso.h"
#include "pomodoro_bg.h"
#include "ui.h"
#include "screenPJ.h"
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

/* Muestra un error aunque SDL no este cargado */
static void fatal(const char *titulo, const char *msg) {
#ifdef _WIN32
    MessageBoxA(NULL, msg, titulo, MB_OK | MB_ICONERROR);
#else
    fprintf(stderr, "%s: %s\n", titulo, msg);
#endif
}

int
main(int argc, char *argv[]) {
    (void)argc; (void)argv;

#ifdef _WIN32
    MessageBoxA(NULL, "main() iniciado", "DEBUG", MB_OK);
    CreateDirectoryA("saves", NULL);
#endif

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fatal("Error SDL_Init", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        fatal("Error TTF_Init", TTF_GetError());
        return 1;
    }

    cargar_niveles("data/niveles.json");
    cargar_progreso("saves/progreso.json");

    TTF_Font *fuente = TTF_OpenFont("assets/fonts/main.ttf", 16);
    if (!fuente) {
        char msg[256];
        snprintf(msg, sizeof(msg), "No se pudo abrir assets/fonts/main.ttf\n%s", TTF_GetError());
        fatal("Error fuente", msg);
        return 1;
    }

    SDL_Window *ventana = SDL_CreateWindow(
            "PseudoGames",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            800, 600,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
            );
    if (!ventana) {
        fatal("Error ventana", SDL_GetError());
        return 1;
    }

    /* Intentar GPU primero; si falla (WSLg sin OpenGL) usar software */
    SDL_Renderer *renderer = SDL_CreateRenderer(ventana, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
        renderer = SDL_CreateRenderer(ventana, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        fatal("Error renderer", SDL_GetError());
        return 1;
    }

    SDL_ShowWindow(ventana);
    SDL_RestoreWindow(ventana);
    SDL_RaiseWindow(ventana);
    SDL_SetWindowInputFocus(ventana);

    /* Leer el tamaño real de la ventana — sin logical size para evitar
       el blur del escalado interno de SDL. Cada pantalla dibuja directo
       en la resolución nativa. Se re-lee al inicio de cada iteración
       para capturar cambios (maximizar, pantalla completa, resize). */
    int ancho, alto;
    SDL_GetWindowSize(ventana, &ancho, &alto);
    screen_poweron(renderer, ancho, alto);

#ifdef _WIN32
    MessageBoxA(NULL, "screen_poweron OK, entrando a screenMenu", "DEBUG", MB_OK);
#endif

    /* ── intro: mazmorra 3D, una sola vez en la vida ── */
    if (!intro_ya_vista()) {
        screenPJ_intro(renderer, ventana, ancho, alto);
        marcar_intro_vista();
        screen_transition(renderer, ancho, alto);
    }

    int opcion = 0;
    int primera_vez = 1;
    do {
        /* Re-leer resolución real en cada vuelta del loop:
           captura maximize, fullscreen o resize del usuario */
        SDL_GetWindowSize(ventana, &ancho, &alto);

        if (!primera_vez) screen_transition(renderer, ancho, alto);
        primera_vez = 0;
        opcion = screenMenu(renderer, fuente, ancho, alto);
        if (opcion == 0) break;
        screen_transition(renderer, ancho, alto);
        switch (opcion){
            case 1:{
                   int nivel = screenLvLs(renderer, fuente, ancho, alto);
                   if(nivel > 0) {
                        screen_transition(renderer, ancho, alto);
                        screenLvLEditor(renderer, fuente, ancho, alto, nivel);
                   }
                   break;
                   }
            case 2: screenDoc(renderer, fuente, ancho, alto); break;
            case 3: screenPomodoro(renderer, fuente, ancho, alto); break;
            case 4: screenFreeEditor(renderer, fuente, ancho, alto, 0); break;
            case 5: screenLvLs(renderer, fuente, ancho, alto); break;
            case 6: screenSoluciones(renderer, fuente, ancho, alto); break;
            case 7: screenConfig(renderer, fuente, ancho, alto, ventana); break;
        }
    }while (opcion != 0);
    
    pom_cleanup();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(ventana);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
