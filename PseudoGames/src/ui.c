#include "ui.h"
#include "niveles.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
void
dibujadoTextoColor(SDL_Renderer *renderer, TTF_Font *fuente, const char *texto, int x, int y, SDL_Color color){
    SDL_Surface *sup = TTF_RenderUTF8_Blended(fuente, texto, color);
    if (!sup) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, sup);
    SDL_Rect pos_txt = {x + 10, y + 12, sup->w, sup->h};
    SDL_RenderCopy(renderer, tex, NULL, &pos_txt);
    SDL_FreeSurface(sup);
    SDL_DestroyTexture(tex);
}

void
dibujadoTexto(SDL_Renderer *renderer, TTF_Font *fuente, const char *texto, int x, int y){
    // Config del screenMenu(){
    // texto del botón
    SDL_Color blanco = {255, 255, 255, 255};
    // El suo->w : cuando hago el TTF_RenderText_Solid() devuelve un SDL_Surface * que es una imagen en memoria del texto renderizado. Esa surface tiene propiedades:
    //  sup->w   ← el ancho en píxeles que ocupa ese texto
    //  sup->h   ← el alto en píxeles

    //  ¿Por qué usarlo? Porque cada label tiene distinto largo:
    //  "1. Jugar"    →  sup->w = ~80px
    //  "3. Progreso" →  sup->w = ~120px

    //  Si ponés un número fijo te puede quedar cortado o con espacio de más. Usando
    //  sup->w el rectángulo del texto se ajusta exacto a lo que mide ese string.
    //  SDL_Surface *sup apunta a una estructura que SDL crea en memoria con la imagen del texto ya renderizada. 
    SDL_Surface *sup = TTF_RenderUTF8_Blended(fuente, texto, blanco);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, sup);
    SDL_Rect pos_txt = {x + 10, y + 12, sup->w, sup->h};
    SDL_RenderCopy(renderer, tex, NULL, &pos_txt);
    SDL_FreeSurface(sup);
    SDL_DestroyTexture(tex);
    //}

}



/*
 *  char linea[512] = "" — línea 110

  Es el acumulador. Acá vas juntando palabras hasta que no entren más. Empieza vacío "" porque al inicio no tenés nada acumulado todavía.

  ---
  int y_actual = y — línea 114

  y es donde arranca el texto (por ejemplo y=70). y_actual va creciendo cada vez que dibujás una línea y bajás a la siguiente. y no lo tocás, y_actual es el que se mueve.

  ---
  TTF_FontHeight(fuente) + 4 — línea 117

  TTF_FontHeight() te da la altura en píxeles de la fuente — por ejemplo 16px. El +4 es un margen entre líneas para que no queden pegadas. Sin él el texto quedaría así:
  Implementa una funcion
  que reciba un array       ← pegado arriba
  Con +4 queda con espacio respirable. Podés cambiarlo a +6 o +8 si querés más separación.

  ---
  strtok(copia, " \n\r\t") — línea 119

  strtok parte un string en pedazos usando un separador — en este caso el espacio " ".

  Primera llamada: strtok(copia, " \n\r\t") — le pasás el string, te devuelve la primera palabra.

  Dentro del while, línea 137: strtok(NULL, " \n\r\t") — le pasás NULL para decirle "seguí desde donde quedaste", te devuelve la siguiente palabra.

  Cuando no quedan más palabras devuelve NULL y el while termina.

  strtok("Implementa una funcion", " ")
    → primera llamada → "Implementa"
    → strtok(NULL, " \n\r\t") → "una"
    → strtok(NULL, " \n\r\t") → "funcion"
    → strtok(NULL, " \n\r\t") → NULL  ← fin

  ---
  También noto un bug en la línea 122 — el operador ternario está mal:

  (linea[0] != '\0') ? " " : " "   ← ambos casos son " ", no hace nada

  Debería ser:
  (linea[0] != '\0') ? " " : ""   ← si linea tiene algo, agrega espacio; si está vacía, no

  Corregí eso.*/



































//===
/* La logica que se usa simplemente es palabra por palabra, probando si entra en el box
 *
 * El Enunciado tiene que decir: "Implementa una funcion que reciba un array"
 * 
 * ----Iteracion 1----
 *  palabra = "Implementa"
 *  prueba = "Implementa"  <- entra en max_ancho? ==> SI
 *  linea = "Implementa"
 * 
 * ----Iteracion 2----
 *  palabra = "una"
 *  prueba = "Implementa una"  <- entra en max_ancho? ==> SI
 *  linea = "Implementa una"
 * 
 * ----Iteracion 3----
 *  palabra = "funcion"
 *  prueba = "Implementa una funcion" <- entra en max_ancho? ==> SI
 *  linea = "Implementa una funcion"        
 * 
 * ----Iteracion 4----
 *  palabra = "reciba"
 *  prueba = "que reciba"             <- entra en max_ancho? ==> SI
 *          -> Dibujar "Implementa una funcion"      en y = 70
 *          -> y_actual += alto_linea
 *          -> linea = "que"          <- empieza linea nueva
 * 
 * ----Iteracion 5----
 *  palabra = "reciba"
 *  prueba = "que reciba"           <- entra en max_ancho? ==> SI
 *  linea = "que reciba"
 *
 * ----Iteracion 6----> asi sucesivamente hasta que termine la consigna, lo importante es que va almacenandoce y preguntando palabra por palabra si cabe dentro de max_ancho.
 *
 */ 
//===


void
dibujadoTextoMultilineaColor(SDL_Renderer *renderer, TTF_Font *fuente, const char *texto, int x, int y, int max_ancho, SDL_Color color)
{
    char copia[2048];
    strncpy(copia, texto, sizeof(copia) - 1);
    copia[sizeof(copia) - 1] = '\0';

    char linea[512] = "";
    int  y_actual   = y;
    int  alto_linea = TTF_FontHeight(fuente) + 4;
    char *palabra   = strtok(copia, " \n\r\t");

    while (palabra != NULL) {
        char prueba[512];
        snprintf(prueba, sizeof(prueba), "%s%s%s", linea, (linea[0] != '\0') ? " " : "", palabra);

        int ancho_prueba;
        TTF_SizeUTF8(fuente, prueba, &ancho_prueba, NULL);

        if (ancho_prueba > max_ancho && linea[0] != '\0') {
            dibujadoTextoColor(renderer, fuente, linea, x, y_actual, color);
            y_actual += alto_linea;
            snprintf(linea, sizeof(linea), "%s", palabra);
        } else {
            snprintf(linea, sizeof(linea), "%s", prueba);
        }
        palabra = strtok(NULL, " \n\r\t");
    }

    if (linea[0] != '\0')
        dibujadoTextoColor(renderer, fuente, linea, x, y_actual, color);
}

void
dibujadoTextoMultiLinea(SDL_Renderer *renderer, TTF_Font *fuente, const char *texto, int x, int y, int max_ancho)
{
    char copia[2048];
    strncpy(copia, texto, sizeof(copia) - 1);
    copia[sizeof(copia) - 1] = '\0';
    
    // Para que se crea char linea [512]?
    //      
    char linea [512] = "";
    
    // Este es el actual de pixeles en y para que?
    //
    int y_actual = y;
    // Esto lo que hace segun lo que entiendo el alto de la fuente de fuente +4 es asignado al alto_linea porque + 4?
    //
    int alto_linea = TTF_FontHeight(fuente) + 4;
    // que es strtok()?
    char *palabra = strtok(copia, " \n\r\t");
    while (palabra != NULL){
        char prueba[512];
        // el "" devuelve un valor? -> " " es nada?
        snprintf(prueba, sizeof(prueba), "%s%s%s", linea, (linea[0] != '\0') ? " " : "", palabra);
        
        int ancho_prueba;
        // TTF_SizeUTF8()
        //      Te dice cuantos pixeles ocupa un string con esa fuente. No contas caracteres, medis pixeles reales. Asi funciona bien con cualquier fuente y tamano.
        TTF_SizeUTF8(fuente, prueba, &ancho_prueba, NULL);

        if (ancho_prueba > max_ancho && linea[0] != '\0'){
            dibujadoTexto(renderer, fuente, linea, x, y_actual);
            y_actual += alto_linea;
            snprintf(linea, sizeof(linea), "%s", palabra);
          } else {
              snprintf(linea, sizeof(linea), "%s", prueba);
          }

          palabra = strtok(NULL, " \n\r\t");
      }

      if (linea[0] != '\0')
          dibujadoTexto(renderer, fuente, linea, x, y_actual);
}

void
screen_poweron(SDL_Renderer *renderer, int ancho, int alto)
{
    int cy = alto / 2;
    srand(SDL_GetTicks());

    // FASE 1 (frames 0-24): aparece el punto/linea brillante en el centro
    for (int f = 0; f < 25; f++) {
        float p = (float)f / 24.0f;   // 0 → 1

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // la linea crece en ancho con aceleracion (p^2)
        int lw = (int)(p * p * ancho);
        int lx = (ancho - lw) / 2;
        int lh = 2 + (int)(p * 5);

        // halo alrededor de la linea (bloom)
        for (int g = 10; g >= 1; g--) {
            Uint8 a = (Uint8)(40.0f * p / g);
            SDL_SetRenderDrawColor(renderer, 200, 230, 200, a);
            SDL_Rect halo = {lx - g*3, cy - lh/2 - g*2,
                             lw + g*6, lh + g*4};
            SDL_RenderFillRect(renderer, &halo);
        }

        // nucleo brillante
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, (Uint8)(220 * p));
        SDL_Rect linea = {lx, cy - lh/2, lw, lh};
        SDL_RenderFillRect(renderer, &linea);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_RenderPresent(renderer);
        SDL_Delay(14);
    }

    // FASE 2 (frames 0-50): la linea se abre verticalmente
    for (int f = 0; f < 51; f++) {
        float p  = (float)f / 50.0f;
        // ease-out: empieza rapido, frena al llegar al borde
        float ep = 1.0f - (1.0f - p) * (1.0f - p);

        int half_h = (int)(ep * (alto / 2));
        int top    = cy - half_h;
        int bot    = cy + half_h;
        if (top < 0)    top = 0;
        if (bot > alto) bot = alto;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // zona visible: fondo verde oscuro (como el menu)
        SDL_SetRenderDrawColor(renderer, 8, 14, 8, 255);
        SDL_Rect visible = {0, top, ancho, bot - top};
        SDL_RenderFillRect(renderer, &visible);

        // algo de ruido en la zona visible
        if (rand() % 2 == 0) {
            int ny = top + rand() % (bot - top + 1);
            Uint8 v = (Uint8)(rand() % 120);
            SDL_SetRenderDrawColor(renderer, v, v, v, 80);
            SDL_Rect nr = {0, ny, ancho, 1};
            SDL_RenderFillRect(renderer, &nr);
        }

        // bordes brillantes (las "cortinas" que se abren)
        for (int g = 8; g >= 0; g--) {
            Uint8 a = (Uint8)(120 / (g + 1));
            SDL_SetRenderDrawColor(renderer, 180, 220, 180, a);
            if (top - g >= 0) {
                SDL_Rect et = {0, top - g, ancho, 1};
                SDL_RenderFillRect(renderer, &et);
            }
            if (bot + g < alto) {
                SDL_Rect eb = {0, bot + g, ancho, 1};
                SDL_RenderFillRect(renderer, &eb);
            }
        }
        // nucleo del borde
        SDL_SetRenderDrawColor(renderer, 220, 255, 220, 200);
        SDL_Rect et = {0, top,     ancho, 2};
        SDL_Rect eb = {0, bot - 2, ancho, 2};
        SDL_RenderFillRect(renderer, &et);
        SDL_RenderFillRect(renderer, &eb);

        // barras negras (lo que todavia no se ve)
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        if (top > 0) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_Rect tb = {0, 0, ancho, top};
            SDL_RenderFillRect(renderer, &tb);
        }
        if (bot < alto) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_Rect bb = {0, bot, ancho, alto - bot};
            SDL_RenderFillRect(renderer, &bb);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(14);
    }

    // FASE 3 (frames 0-30): pantalla completa, parpadeos hasta estabilizarse
    for (int f = 0; f < 31; f++) {
        float p = (float)f / 30.0f;

        // parpadeos al principio, se van calmando
        int parpadeo = (f % 4 == 0 && p < 0.6f) ? 50 : 0;
        SDL_SetRenderDrawColor(renderer,
            8  + parpadeo,
            14 + parpadeo,
            8  + parpadeo, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        int n_ruido = (int)((1.0f - p) * 20);
        for (int i = 0; i < n_ruido; i++) {
            int ry = rand() % alto;
            int rw = ancho / 2 + rand() % (ancho / 2);
            int rx = rand() % (ancho / 4);
            Uint8 v = (Uint8)(rand() % 160);
            SDL_SetRenderDrawColor(renderer, v, v, v, 90);
            SDL_Rect nl = {rx, ry, rw, 1};
            SDL_RenderFillRect(renderer, &nl);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

void
screen_transition(SDL_Renderer *renderer, int ancho, int alto)
{
    // Capturar el frame actual antes de distorsionarlo
    SDL_Surface *snap_surf = SDL_CreateRGBSurface(
        0, ancho, alto, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!snap_surf) return;

    SDL_RenderReadPixels(renderer, NULL,
        SDL_PIXELFORMAT_ARGB8888, snap_surf->pixels, snap_surf->pitch);

    SDL_Texture *snap = SDL_CreateTextureFromSurface(renderer, snap_surf);
    SDL_FreeSurface(snap_surf);
    if (!snap) return;

    // Fases del efecto: 0-1 = distorsion creciente, 0.7-1 = flash blanco
    int FRAMES = 22;
    srand(SDL_GetTicks());

    for (int f = 0; f < FRAMES; f++) {
        float t = (float)f / (float)FRAMES;  // 0.0 → 1.0

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // --- Desgarro horizontal: dibujar el snapshot en bandas desplazadas ---
        float intensidad = sinf(t * 3.14159f) * 120.0f; // crece y baja
        int band_h = 3 + rand() % 6;

        for (int y = 0; y < alto; y += band_h) {
            // desplazamiento aleatorio ponderado por la intensidad
            int shift = (int)(((float)(rand() % 201) - 100.0f) / 100.0f * intensidad);

            // algunas bandas quedan en negro (rasgaduras)
            if (rand() % 8 == 0) continue;

            SDL_Rect src = {0,     y, ancho,         band_h};
            SDL_Rect dst = {shift, y, ancho + abs(shift), band_h};
            SDL_RenderCopy(renderer, snap, &src, &dst);
        }

        // --- Ruido: lineas horizontales de estatica ---
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        int n_ruido = (int)(t * 30);
        for (int i = 0; i < n_ruido; i++) {
            int ry  = rand() % alto;
            int rh  = 1 + rand() % 3;
            int rw  = ancho / 3 + rand() % (ancho * 2 / 3);
            int rx  = rand() % (ancho / 4);
            Uint8 v = (Uint8)(rand() % 256);
            SDL_SetRenderDrawColor(renderer, v, v, v, 180 + rand() % 75);
            SDL_Rect rl = {rx, ry, rw, rh};
            SDL_RenderFillRect(renderer, &rl);
        }

        // --- Linea de sincronizacion: barre de arriba a abajo ---
        int sync_y = (int)(t * alto);
        SDL_SetRenderDrawColor(renderer, 200, 200, 220, 60);
        SDL_Rect sync_line = {0, sync_y, ancho, 2};
        SDL_RenderFillRect(renderer, &sync_line);

        // --- Flash blanco al final ---
        if (t > 0.72f) {
            float flash = (t - 0.72f) / 0.28f;
            SDL_SetRenderDrawColor(renderer, 255, 255, 255,
                (Uint8)(flash * flash * 255));
            SDL_RenderFillRect(renderer, NULL);
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_RenderPresent(renderer);
        SDL_Delay(14);
    }

    SDL_DestroyTexture(snap);
}





