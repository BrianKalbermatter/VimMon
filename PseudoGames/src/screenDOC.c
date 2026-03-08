#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"
#include "niveles.h"

int
screenDoc(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto){
    // fgets(str, count, stream);
    //      - Cuando llamo a fgets();, lee una linea y la guarda en una variable designada
    //      - Necesita los 3 parametros:
    //              En la practica los memorizaria rapido porque siempre se ven iguales:
    //              fgets(donde_guardo, tamano_del_donde, de_donde_leo);
    //          stdin - en vez de leer de un archivo, lee lo que el usuario tipea:
    //          fgets(linea, 512, stdin); // lee del teclado
    //          fgets(); // lee de un archivo
    //
    //          char linea[512];
    //          fgets(linea, 512, archivo);
    //          - linea — el array donde guarda lo que leyó
    //          - 512 — el máximo de caracteres que puede leer (para no pasarse del array)
    //          - archivo — el archivo que está leyendo
    
    // Estado de Navegacion
    int capitulo_sel = 0;
    int scroll = 0;
    SDL_Event evento;


    // Datos del archivo
    // el char lineas 3200 revienta porque es 1.6MB en el stack y explota fue cambiado por static
    static char lineas[3200][512];
    // Porque pasa esto?
    // En C, las variables locales viven en el stack. El stack tiene un limite chico, aproximadamente en Linux
    // Con static la variable vive en el data segment - una zona de memoria sin ese limite chico.
    // Única diferencia práctica: los static locales mantienen su valor entre llamadas
    // a la función. Para vos no importa porque total_lineas = 0 la reseteás vos mismo cada vez.
    

    int total_lineas =0;
    
    // Indice de capitulos
    typedef struct {
        char titulo[256];
        int linea_inicio;
    } Capitulo;
    
    Capitulo capitulos[20];
    int total_capitulos = 0;


    // Leer el archivo
    // Abre el archivo en modo lectura ("r"). Devuelve NULL si no lo encuentra.
    FILE *f = fopen("data/wiki.txt", "r");
    printf("DEBUG fopen: %p\n", (void*)f);
    fflush(stdout);

    // perror imprime el error real del sistema operativo. 
    if (f == NULL) { perror("wiki"); return 0;}
    
    // Cada vuelta lee una linea y la guarda en lineas[0], lineas[1], etc. Cuando no quedan mas lineas, fgets devuelve NULL y para.
    while (fgets(lineas[total_lineas], 512, f) != NULL){
        total_lineas++;
    }
    // Siempre cerrar el archivo cuando termina de verlo.
    fclose(f);

    // --- detectar capítulos ---
    for (int i = 0; i < total_lineas; i++) {
        if (strncmp(lineas[i], "CAPITULO", 8) == 0) {
            strncpy(capitulos[total_capitulos].titulo, lineas[i], 255);
            capitulos[total_capitulos].linea_inicio = i;
            total_capitulos++;
        }
    }
    while (1) {
      while (SDL_PollEvent(&evento)) {
          if (evento.type == SDL_QUIT) return 0;
          if (evento.type == SDL_KEYDOWN) {
              switch (evento.key.keysym.sym) {
                  case SDLK_ESCAPE: return 0;
                  case SDLK_UP:   capitulo_sel--; if (capitulo_sel < 0) capitulo_sel = 0; scroll = 0;
                  break;
                  case SDLK_DOWN: capitulo_sel++; if (capitulo_sel >= total_capitulos) capitulo_sel = total_capitulos - 1; scroll = 0; break;
                  case SDLK_PAGEUP:   scroll -= 5; if (scroll < 0) scroll = 0; break;
                  case SDLK_PAGEDOWN: scroll += 5; break;
              }
          }
      }


      // fondo
      SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
      SDL_RenderClear(renderer);

      // panel izquierdo - índice
      int panel_izq_w = 320;
      SDL_Rect panel_izq = {0, 0, panel_izq_w, alto};
      SDL_SetRenderDrawColor(renderer, 30, 30, 50, 255);
      SDL_RenderFillRect(renderer, &panel_izq);

      // panel derecho - contenido
      SDL_Rect panel_der = {panel_izq_w, 0, ancho - panel_izq_w, alto};
      SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
      SDL_RenderFillRect(renderer, &panel_der);

      // línea divisoria
      SDL_SetRenderDrawColor(renderer, 80, 80, 120, 255);
      SDL_RenderDrawLine(renderer, panel_izq_w, 0, panel_izq_w, alto);
      

      // dibujar índice
      for (int i = 0; i < total_capitulos; i++) {
          int item_y = 20 + (i * 30);

          if (i == capitulo_sel) {
              SDL_Rect highlight = {0, item_y - 4, panel_izq_w, 28};
              SDL_SetRenderDrawColor(renderer, 60, 60, 120, 255);
              SDL_RenderFillRect(renderer, &highlight);
          }

          dibujadoTexto(renderer, fuente, capitulos[i].titulo, 10, item_y);
      }

      // dibujar contenido del capítulo seleccionado
      int linea_desde = capitulos[capitulo_sel].linea_inicio;
      int linea_hasta = (capitulo_sel + 1 < total_capitulos)
                        ? capitulos[capitulo_sel + 1].linea_inicio
                        : total_lineas;

      int alto_linea = TTF_FontHeight(fuente) + 2;
      int y_texto = 20;

      for (int i = linea_desde + scroll; i < linea_hasta; i++) {
          if (y_texto > alto - 20) break;
          if (lineas[i][0] != '\n' && lineas[i][0] != '\0')
              dibujadoTexto(renderer, fuente, lineas[i], panel_izq_w + 10, y_texto);
          y_texto += alto_linea;
      }

      SDL_RenderPresent(renderer);
    }
    
    return 0;
}

