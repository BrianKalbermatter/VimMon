#include <stdio.h>
#include <SDL2/SDL.h>
// Esto se debe enpotrar en el bus
SDL_Init(SDL_INIT_VIDEO);
int
main(){
    
    // Ventana
    
    SDL_Window *win = SDL_CreateWindow(" ", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
    if(SDL_Window = NULL){
      SDL_GetError();
    }
}
