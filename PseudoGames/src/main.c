#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <pty.h> // Es una libreria que tiene la funcion forkpty()
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include "niveles.h"
#include "progreso.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>

int 
main(void) {
	SDL_Init(SDL_INIT_VIDEO);
    TTF_Init(); // Inicializa la libreria SDL2_ttf, igual que SDL_Init pero para fuentes
    
    cargar_niveles("data/niveles.json");
    TTF_Font *fuente = TTF_OpenFont("assets/fonts/main.ttf", 16);
    SDL_Window *ventana = SDL_CreateWindow(
            "PseudoGames",
            SDL_WINDOWPOS_CENTERED, // Centra la pantalla automaticamente
            SDL_WINDOWPOS_CENTERED,
            800, 600,
             SDL_WINDOW_FULLSCREEN_DESKTOP  // ← pantalla completa
            );
    
    SDL_Renderer * renderer = SDL_CreateRenderer(ventana, -1, 0); // 0: flags(0=default)
                                                                 // -1: indice del driver (el mejor disponible)
                                                                 // screen en que ventana dibuja
                                                                 // La ventana puntero que guarda esa direccion.
	

    int ancho, alto;
    SDL_GetWindowSize(ventana, &ancho, &alto);
    int opcion = 0;
    do {
	    opcion = screenMenu(renderer, fuente, ancho, alto);
        switch (opcion){
            case 1:{ 
                   int nivel = screenLvLs(renderer, fuente, ancho, alto);
                   if(nivel > 0)
                        screenLvLEditor(renderer, fuente, ancho, alto, nivel);
                   break;
                   }
            case 2: screenDoc(renderer, fuente, ancho, alto); break;
            case 3: break;// screenPomodoro(); break;
            case 4: break;// screenFreeEditor(); break;              
            case 5: screenLvLs(renderer, fuente, ancho, alto); break;
            case 6: screenSoluciones(renderer, fuente, ancho, alto); break;
            case 0: break;// SALIR
        }
    }while (opcion != 0);
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(ventana);
    TTF_Quit();
    SDL_Quit();
    return 0; 
}
