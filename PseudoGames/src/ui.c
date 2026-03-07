#include "ui.h"
#include "niveles.h"
#include <string.h>
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
    SDL_Surface *sup = TTF_RenderText_Solid(fuente, texto, blanco);
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
  strtok(copia, " ") — línea 119

  strtok parte un string en pedazos usando un separador — en este caso el espacio " ".

  Primera llamada: strtok(copia, " ") — le pasás el string, te devuelve la primera palabra.

  Dentro del while, línea 137: strtok(NULL, " ") — le pasás NULL para decirle "seguí desde donde quedaste", te devuelve la siguiente palabra.

  Cuando no quedan más palabras devuelve NULL y el while termina.

  strtok("Implementa una funcion", " ")
    → primera llamada → "Implementa"
    → strtok(NULL, " ") → "una"
    → strtok(NULL, " ") → "funcion"
    → strtok(NULL, " ") → NULL  ← fin

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
    char *palabra = strtok(copia, " ");
    while (palabra != NULL){
        char prueba[512];
        // el "" devuelve un valor? -> " " es nada?
        snprintf(prueba, sizeof(prueba), "%s%s%s", linea, (linea[0] != '\0') ? " " : "", palabra);
        
        int ancho_prueba;
        // TTF_SizeText()
        //      Te dice cuantos pixeles ocupa un string con esa fuente. No contas caracteres, medis pixeles reales. Asi funciona bien con cualquier fuente y tamano.
        TTF_SizeText(fuente, prueba, &ancho_prueba, NULL);

        if (ancho_prueba > max_ancho && linea[0] != '\0'){
            dibujadoTexto(renderer, fuente, linea, x, y_actual);
            y_actual += alto_linea;
            snprintf(linea, sizeof(linea), "%s", palabra);
          } else {
              snprintf(linea, sizeof(linea), "%s", prueba);
          }

          palabra = strtok(NULL, " ");
      }

      if (linea[0] != '\0')
          dibujadoTexto(renderer, fuente, linea, x, y_actual);


}

    



