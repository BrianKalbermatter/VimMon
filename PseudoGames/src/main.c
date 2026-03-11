#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include "niveles.h"
#include "progreso.h"
#include "pomodoro_bg.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>

int
main(int argc, char *argv[]) {
    (void)argc; (void)argv;
	SDL_Init(SDL_INIT_VIDEO);
    TTF_Init(); // Inicializa la libreria SDL2_ttf, igual que SDL_Init pero para fuentes
    
    cargar_niveles("data/niveles.json");
    cargar_progreso("saves/progreso.json");
    TTF_Font *fuente = TTF_OpenFont("assets/fonts/main.ttf", 16);
    SDL_Window *ventana = SDL_CreateWindow(
            "PseudoGames",
            SDL_WINDOWPOS_CENTERED, // Centra la pantalla automaticamente
            SDL_WINDOWPOS_CENTERED,
            800, 600,
             SDL_WINDOW_FULLSCREEN_DESKTOP  // ← pantalla completa
            );
    
    SDL_Renderer *renderer = SDL_CreateRenderer(ventana, -1, 0);

    // Traer la ventana al frente y darle foco de teclado/mouse sin necesitar un click
    SDL_RaiseWindow(ventana);
    SDL_SetWindowInputFocus(ventana);

    int ancho, alto;
    SDL_GetWindowSize(ventana, &ancho, &alto);
    screen_poweron(renderer, ancho, alto);

    int opcion = 0;
    int primera_vez = 1;
    do {
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
