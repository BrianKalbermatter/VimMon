#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"
#include "niveles.h"
int
screenMenu(SDL_Renderer *renderer,TTF_Font *fuente, int ancho, int alto){
    int corriendo = 1;
    SDL_Event evento;
    

    int btn_w = 300;
    int btn_h = 50;
    int btn_x = (ancho - btn_w) / 2;
    while(corriendo){
        while(SDL_PollEvent(&evento)){
            // Caso 1: cerro la ventana con la X del sistema operativo
            if (evento.type == SDL_QUIT) return 0;
            // Caso 2: se presiono la tecla para salir!
            if(evento.type == SDL_KEYDOWN){
                switch (evento.key.keysym.sym){
                    case SDLK_1: return 1;
                    case SDLK_2: return 2;
                    case SDLK_3: return 3;
                    case SDLK_4: return 4;
                    case SDLK_5: return 5;
                    case SDLK_6: return 6;
                }
            }
        }
    
        // Fondo
        SDL_SetRenderDrawColor(renderer, 20, 20,30, 255);
        SDL_RenderClear(renderer);
        int mx, my; // Posiciones del mouse
        SDL_GetMouseState(&mx, &my);
        // Los 4 Botones
        char *labels[7] = {"Jugar","DOC","Pomodoro","Editor Libre","Seleccion de Nivel", "Soluciones","Salir"};
        // Renderizado
    


        for (int i = 0; i < 7; i++) {
            int btn_y = (alto / 2) - 80 + (i * 70);
            
            // Para saber si el mouse esta sobre el boton
            int hover = (mx >= btn_x && mx <= btn_x + btn_w && my >= btn_y && my <= btn_y + btn_h);

            if (hover)
                SDL_SetRenderDrawColor(renderer, 0, 120, 200, 255); // azul mas claro
            else
                SDL_SetRenderDrawColor(renderer, 0, 78, 152, 255);  // azul normal
            
            SDL_Rect btn = {btn_x, btn_y, btn_w, btn_h};
            SDL_RenderFillRect(renderer, &btn);
            dibujadoTexto(renderer, fuente, labels[i], btn_x, btn_y);
        

            if (evento.type == SDL_MOUSEBUTTONDOWN) {
                int cx = evento.button.x;
                int cy = evento.button.y;

                if (cx >= btn_x && cx <= btn_x + btn_w && cy >= btn_y && cy <= btn_y + btn_h)
                    return (i == 6)? 0 : i + 1;
            }
        }  // cierra for

        SDL_RenderPresent(renderer);
    }  // cierra while(corriendo)

    return 0;
}
    

