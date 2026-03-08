#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"
#include "niveles.h"
int
screenLvLs(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto){
    SDL_Event evento;
    int total = total_niveles();
    
    int btn_w = 400;
    int btn_h = 55;
    int btn_x = (ancho - btn_w) / 2;

    while (1) {
          while (SDL_PollEvent(&evento)) {
              if (evento.type == SDL_QUIT) return 0;
              if (evento.type == SDL_KEYDOWN) {
                  switch (evento.key.keysym.sym) {
                      case SDLK_ESCAPE: return 0;
                      case SDLK_1: return 1;
                      case SDLK_2: return 2;
                      case SDLK_3: return 3;
                  }
              }
          }
          SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
          SDL_RenderClear(renderer);

          int mx, my;
          SDL_GetMouseState(&mx, &my);

          for (int i = 0; i < total; i++) {
              Nivel *n = obtener_nivel(i + 1);
              if (!n) continue;

              int btn_y = (alto / 2) - 80 + (i * 70);
              int hover = (mx >= btn_x && mx <= btn_x + btn_w &&
                           my >= btn_y && my <= btn_y + btn_h);

              SDL_SetRenderDrawColor(renderer,
                  hover ? 0 : 0,
                  hover ? 120 : 78,
                  hover ? 200 : 152,
                  255);

              SDL_Rect btn = {btn_x, btn_y, btn_w, btn_h};
              SDL_RenderFillRect(renderer, &btn);

              char label[256];
              //snprintf()
              //        sirve para construir un string formateado y guardarlo en una variable
              //        Como tengo dos variables por separado
              //        n->numero => 1
              //        n->titulo => "Variable y Tipos"
              //        Necesito solo un string para pasarle a dibujado texto : [1] Variables y Tipos.
              //        Con snprintf:
              //        char label[256];
              //        snprintf(label, sizeof(label), "[%d] %s", n->numero, n->titulo);

              //        - label — el array donde se guarda el resultado
              //        - sizeof(label) — el tamaño máximo, para que no se pase del array (seguridad)
              //        - "[%d] %s" — el formato: %d es un entero, %s es un string
              //        - n->numero, n->titulo — los valores que van en cada %
              //
              //        Resultado en label: "[1] Variables y tipos"


              snprintf(label, sizeof(label), "[%d] %s", n->numero, n->titulo);
              dibujadoTexto(renderer, fuente, label, btn_x, btn_y);

              if (evento.type == SDL_MOUSEBUTTONDOWN) {
                  int cx = evento.button.x;
                  int cy = evento.button.y;
                  if (cx >= btn_x && cx <= btn_x + btn_w &&
                      cy >= btn_y && cy <= btn_y + btn_h)
                      return i + 1;
              }
          }

          SDL_RenderPresent(renderer);
    }
    


    return 0;
}


int
screenLvLEditor(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto, int nivel_num){
    
    // forkpty() hace 3 cosas en una: 
    // 1. Crea un PTY - Una terminal falsa, como un tubo bidireccional con superpoderes de terminal.
    // 2. Hace fork() - divide el proceso en dos:
    // mi programa (padre)
    // └──proceso hijo(nuevo)
    // 3. Conecta el PTY al hijo - el hijo tiene stdin/stdout conectados al PTY. Todo lo que el hijo escribe, el padre lo puede leer por fd_pty

//    int fd_pty; // Crea una variable para el proceso de hijo bash deja de ser mi programa y pasa a ser bash corriendo bim.sh
                // file descriptor del PTY - por aca leo. escribo lo que bash produce
//    struct winsize ws = {24, 80, 0, 0}; // 24 filas, 80 columnas
                                        
    // pid_t pid es: Sirve para guardar ID de un proceso, porque cada proceso que corre en linux tiene un numero unico El pid Process ID. Tambien conocido como el DNI del proceso... basicamente es un PID int(1111)
    // forkpty()
    // en el struct creo una ventana de tamano de tanto por tanto...
//    pid_t pid = forkpty(&fd_pty, NULL, NULL, &ws);

//  if (pid == 0) {
      // proceso hijo - acá corre bim.sh
//      chdir("scripts/editorBim");
      //chdir: Cambia el directorio de trabajo del proceso hijo como hacer "cd script/editorBim" en la terminal.
//      execlp("bash", "bash", "bim.sh", "archivoPrueba.txt", NULL);
      //execlp: reemplaza el proceso hijo con bash, deja de ser mi programa y pasa a ser bash corriendo bim.sh
      //el NULL al final marca el fin de los argumentos
//      exit(1);
//  }
  // leer output del PTY y printearlo en consola (para verificar)
  //===========================
  // char buf [4096]:
  //    Es un array de caracteres - 4096, cada una guarda un caracter. 
  //    buf[0] = '\e'
  //    buf[1] = '['
  //    buf[2] = '3'
  //    buf[3] = '8'
  //    buf[4] = ';'
  //    ...
  //    buf[n] = '\0'  ← fin del string
  //    Es lo mismo que &buf[0].
  //    Y un puntero char *p = buf apunta al primer carácter del array. Es lo mismo que
  //    &buf[0].
  //  
  //    p → buf[0] → '\e'
  //    p++ → buf[1] → '['
  //    p++ → buf[2] → '3'
  //    
  //    Y porque 4096 y no 1111 por ejemplo?
  //    Es una convención — 4096 es una potencia de 2 (2¹²).
  //    2¹  = 2
  //    2²  = 4
  //    2³  = 8
  //    2⁴  = 16
  //    2⁸  = 256
  //    2¹²  = 4096
  //    2¹⁶ = 65536

  //    Los sistemas operativos y hardware trabajan internamente con potencias de 2.
  //    Usar 4096 es más eficiente que 1111 porque se alinea con cómo la memoria está organizada.

  // Para tu caso — 4096 caracteres es más que suficiente para guardar un frame completo de bim.sh.
  //
  //===========================
  // que hace el fflush?
  // ->
  
  //char buf[4096];
  //int n = read(fd_pty, buf, sizeof(buf));
  //buf[n] = '\0';
  //printf("%s", buf);
  //fflush(stdout);
  // hacer el fd no bloqueante - sin esto el while se congela esperando a bim.sh
/*  fcntl(fd_pty, F_SETFL, O_NONBLOCK);
    while(corriendo) {
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) corriendo = 0;
            if (evento.type == SDL_KEYDOWN)
                if (evento.key.keysym.sym == SDLK_ESCAPE) corriendo = 0;
        }
         SDL_SetRenderDrawColor(renderer, 0, 0, 0 , 255); // Dibuja un color
         SDL_RenderClear(renderer);
         
          // Barra de titulo - arriba de todo
  SDL_Rect barra_titulo = {0, 0, ancho, 30};
  SDL_SetRenderDrawColor(renderer, 0, 78, 152, 255);
  SDL_RenderFillRect(renderer, &barra_titulo);
  // Texto en la barra de titulo
  SDL_Color blanco = {255, 255, 255, 255};
  SDL_Surface *sup = TTF_RenderText_Solid(fuente, "PseudoGames", blanco);
  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, sup);
  SDL_Rect pos_titulo = {10, 5, 150, 20};
  SDL_RenderCopy(renderer, tex, NULL, &pos_titulo);
  SDL_FreeSurface(sup);
  SDL_DestroyTexture(tex);

         // Panel izquierdo - consigna
         SDL_Rect panel_izq = {0, 30, ancho/2, alto-30};
  SDL_SetRenderDrawColor(renderer, 236, 233, 216, 255);
  SDL_RenderFillRect(renderer, &panel_izq);

  // Panel derecho - editor
  SDL_Rect panel_der = {ancho/2, 30, ancho/2, alto-30};
  SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
  SDL_RenderFillRect(renderer, &panel_der);

  // Línea divisoria
  SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
  SDL_RenderDrawLine(renderer, ancho/2, 0, ancho/2, alto);

  // leer output de bim.sh
  char buf[4096];
  int n = read(fd_pty, buf, sizeof(buf) - 1);
  if (n > 0) {
      buf[n] = '\0';
      char *p = buf;
      while (*p != '\0') {
          if (*p == 27) {
              // es un codigo ANSI - por ahora lo saltamos
              while (*p != 'm' && *p != '\0') p++;
          } else {
              // es texto normal - por ahora lo printeamos
              printf("%c", *p);
          }
          p++;
      }
      fflush(stdout);
  }
}
*/
 SDL_Event evento;
      // obtener_nivel(nivel_num) da el Nivel * con el titulo y enunciado
      // El enunciado puede ser largo - por ahora lo muestro en una sola linea, despues lo hacemos multilinea
      Nivel *n = obtener_nivel(nivel_num);
      if (!n) return 0;

      while (1) {
          while (SDL_PollEvent(&evento)) {
              if (evento.type == SDL_QUIT) return 0;
              if (evento.type == SDL_KEYDOWN)
                  if (evento.key.keysym.sym == SDLK_ESCAPE) return 0;
          }

          SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
          SDL_RenderClear(renderer);

          // Barra de titulo
          SDL_Rect barra = {0, 0, ancho, 30};
          SDL_SetRenderDrawColor(renderer, 0, 78, 152, 255);
          SDL_RenderFillRect(renderer, &barra);
          dibujadoTexto(renderer, fuente, "PseudoGames", 0, 0);

          // Panel izquierdo - consigna
          SDL_Rect panel_izq = {0, 30, ancho/2, alto - 30};
          SDL_SetRenderDrawColor(renderer, 200, 195, 178, 255);
          SDL_RenderFillRect(renderer, &panel_izq);

          // Panel derecho - editor
          SDL_Rect panel_der = {ancho/2, 30, ancho/2, alto - 30};
          SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
          SDL_RenderFillRect(renderer, &panel_der);

          // Linea divisoria
          SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
          SDL_RenderDrawLine(renderer, ancho/2, 0, ancho/2, alto);

          // Titulo del nivel en panel izquierdo
          dibujadoTexto(renderer, fuente, n->titulo, 10, 40);
          // Enunciado
          dibujadoTexto(renderer, fuente, n->enunciado, 10, 70);

          SDL_RenderPresent(renderer);
      }
    return 0;
}  // cierra screenLvLEditor










int
screenFreeEditor(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto, int nivel_num){
    return 0;
}

