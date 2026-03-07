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

















int
screenSoluciones(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto){
    SDL_Event evento;
    char *archivos[5];
    int total = 0;
    //opendir();
    //  Abre la carpeta como si fuera un archivo
    DIR *dir = opendir("solutions");
    struct dirent *entrada;
    printf("dir: %p\n", (void*)dir);
    if (dir == NULL){
        printf("No encontro la carpeta solutions\n");
        return 0;
    }
    //readdir();
    //  Lee una entrada por vez:
    //  - Guardamos los nombres en el array archivos[]
    //  Cada archivo de solucion que haya es un struct dirent *entrada con d_name
    while ((entrada = readdir(dir)) != NULL){
        if (entrada->d_name[0] == '.') continue;
        archivos[total] = strdup(entrada->d_name);
        total++;
    }
    closedir(dir);

    while (1) {
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) return 0;
            if (evento.type == SDL_KEYDOWN)
                if (evento.key.keysym.sym == SDLK_ESCAPE) return 0;
        }
        SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
        SDL_RenderClear(renderer);

        // Tarjetas de soluciones
        for (int i = 0; i < total; i++) {
        int card_w = 300;
        int card_h = 60;
        int card_x = (ancho - card_w) / 2;
        int card_y = 50 + (i * 80);
        
        

        SDL_Rect card = {card_x, card_y, card_w, card_h};
        SDL_SetRenderDrawColor(renderer, 40, 80, 120, 255);
        SDL_RenderFillRect(renderer, &card);

        dibujadoTexto(renderer, fuente, archivos[i], card_x, card_y);
        }

        // Boton con el SALIR
        SDL_Rect btn_salir = {10, alto - 50, 100, 35};
        SDL_SetRenderDrawColor(renderer, 0, 120, 200, 255); // azul mas claro
        SDL_RenderFillRect(renderer, &btn_salir);
        dibujadoTexto(renderer, fuente, "Salir", 10, alto - 50);

        // Click en salir
        if (evento.type == SDL_MOUSEBUTTONDOWN) {
            int cx = evento.button.x;
            int cy = evento.button.y;
            if (cx >= 10 && cx <= 110 && cy >= alto - 50 && cy <= alto - 15)
                return 0;
  }
        
        SDL_RenderPresent(renderer);
    }
    return 0;
}







// Titulo del GAME + Pantalla con nivel y editor
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
