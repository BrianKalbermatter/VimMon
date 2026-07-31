#ifndef DUNGEON_H
#define DUNGEON_H

// dungeon.h — los tipos de TU juego. Nada de plataforma aca (eso es
// platform.h), nada de cuerpos de funciones (eso es el .c correspondiente).

#include "combate.h"
#include "playerC.h"

#define MAX_ITEMS 50
#define MAX_ROOMS 8
#define MAX_DESC 200

typedef struct {
  char nombre[50];
  char descripcion[MAX_DESC];
  int heal_amount;
  int damage_bonus;
  int value;
} Items;

typedef struct {
  char name[50];
  int hp;
  int max_hp;
  int attack;
  int defense;
  int gold;
  int level;
  int experiense;
  Items inventario[MAX_ITEMS];
  int cont_inventario;
  int habitacion_actual;
} Player;

typedef struct {
  char name[50];
  char descripcion[MAX_DESC];
  int norte, sur, este, oeste; // -1 = la habitacion no existe
  Items items[MAX_ITEMS];
  int item_cont;
  int enemigo_hp;
  char enemigo_nombre[50];
  int enemigo_ataque;
  int limpieza; // 1 si el enemigo murio
} Room;

// El rectangulo por el que el soldado tiene permitido caminar. Los cuatro
// limites son INCLUSIVOS: el soldado puede pararse justo sobre x1 e y1.
//
// Es un dato y no un #define a proposito: asi cada habitacion puede tener su
// propio terreno, y podes cambiarlo en caliente sin recompilar el host.
typedef struct {
  int x0, y0; // esquina superior izquierda
  int x1, y1; // esquina inferior derecha

  // El hueco en la pared derecha por donde entran los enemigos. Son las filas
  // en las que NO se dibuja pared: entre apertura_y0 y apertura_y1, inclusive.
  int apertura_y0, apertura_y1;
} Terreno;

// El estado COMPLETO de la partida. Esto es lo que el host guarda en `mem` y lo
// que sobrevive cada recarga en caliente.
//
// Regla de oro del hot reload: si una variable no esta adentro de este struct,
// se pierde cada vez que guardas el archivo. Nada de globales ni de `static`
// con estado en game.so.
// --- El mundo y la camara ----------------------------------------------------
//
// El mundo es MAS GRANDE que la pantalla. Lo que ves en el mapa grande es una
// ventana que se mueve por encima: la camara. El minimapa muestra el mundo
// entero achicado, con un recuadro marcando por donde anda esa ventana.
//
// Las posiciones de todo (soldados, tesoros, balas) son coordenadas de MUNDO.
// La conversion a pantalla es una resta, y pasa solo al dibujar.
#define MUNDO_ANCHO 240
#define MUNDO_ALTO 80

#define MAX_SOLDADOS 4
#define MAX_ENEMIGOS 24
#define MAX_CUARTELES 4
#define MAX_PROYECTILES 96

// Segundos entre la aparicion de un enemigo y el siguiente.
#define SPAWN_CADA 2.5f

// Margen de paz. El primero es mas largo porque arrancas sin un solo soldado y
// sin cuarteles; los siguientes son para reponer las bajas de la oleada que
// acaba de terminar.
#define PREPARACION_PRIMERA 40.0f
#define PREPARACION_SIGUIENTE 30.0f

// Cuantos enemigos trae cada oleada. Sube de a uno para que la partida no se
// estanque: la oleada 0 trae 5, la 1 trae 6, y asi.
#define ENEMIGOS_OLEADA_BASE 5

// Cada cuanto un cuartel saca un soldado nuevo.
#define CUARTEL_ENTRENA_CADA 5.0f

// --- Recursos ----------------------------------------------------------------

// El enum ordena los recursos y REC_CANTIDAD queda valiendo 8 solo: es el
// truco para que el arreglo y los nombres no se puedan desincronizar. Si
// manianas agregas REC_CARBON antes del final, el arreglo crece con el.
typedef enum {
  REC_MADERA,
  REC_HIERRO,
  REC_COBRE,
  REC_COMIDA,
  REC_ORO,
  REC_TITANIO,
  REC_BREA,
  REC_PIEDRA,
  REC_CARBON,
  REC_HILO,
  REC_CANTIDAD // no es un recurso: es cuantos hay
} TipoRecurso;

// Techo duro de poblacion. Ningun soldado se entrena si llegar a ese numero lo
// haria pasar.
#define POBLACION_MAX 200

typedef struct {
  int cantidad[REC_CANTIDAD]; // todos arrancan en 0
  int poblacion;              // unidades vivas ocupando lugar
} Recursos;

// Los nombres viven al lado del enum para que se lean juntos. Se declara aca y
// se define en dungeon.c: si el arreglo estuviera en el header, cada .c que lo
// incluya tendria su propia copia.
extern const char *const RECURSO_NOMBRE[REC_CANTIDAD];

// --- Tesoros -----------------------------------------------------------------

#define MAX_TESOROS 8

// Un monton de recursos tirado en el mapa, con guardias encima. No se levanta
// hasta que no queda ninguno vivo alrededor: ese es todo el juego.
typedef struct {
  int x, y;
  int tipo;     // cual de los REC_*
  int cantidad; // cuanto suma al levantarlo
  int tomado;   // 1 = ya lo levantaste; queda en el arreglo pero no se dibuja
} Tesoro;

// A que distancia del tesoro cuenta un enemigo como su guardia, y hasta donde
// tiene que llegar un soldado para levantarlo.
#define RADIO_CUSTODIA 6.0f
#define RADIO_RECOGIDA 1.8f

typedef struct {
  int x, y;
  int activo;
  float entrena_espera; // segundos hasta el proximo soldado
  int entrenados;       // cuantos saco; sirve para no apilarlos al salir
} Cuartel;

// El recuadro que se dibuja mientras arrastras el mouse. Se guarda el punto
// donde empezo el arrastre y donde esta ahora: el rectangulo se arma con los
// dos, y por eso podes arrastrar en cualquiera de las cuatro diagonales.
typedef struct {
  int activa; // 1 mientras el boton izquierdo sigue apretado
  int x0, y0; // donde se apreto
  int x1, y1; // donde esta el cursor ahora
} Seleccion;

typedef struct {
  int inicializado; // 0 la primera vez, porque el host entrega `mem` en cero

  // Tamanio real del framebuffer, que el host mide de la terminal y pasa a
  // game_init. TODO el layout sale de aca; FB_ANCHO/FB_ALTO son solo el techo.
  int ancho, alto;
  int piso_alto; // primera fila del panel: el mapa va de 0 a piso_alto-1

  // Esquina superior izquierda del mundo que se esta viendo. La ventana mide
  // ancho x piso_alto.
  int cam_x, cam_y;

  // Paneo: arrastrar con el izquierdo cuando no hay nada seleccionado.
  int pan_activo;
  int pan_x, pan_y; // donde estaba el cursor en el frame anterior
  int hubo_arrastre; // 1 si el cursor se movio entre apretar y soltar

  Terreno terreno;

  Soldado soldados[MAX_SOLDADOS];
  int cant_soldados;

  // Los enemigos muertos NO se sacan del arreglo: quedan con vivo == 0 y su
  // lugar se reusa cuando aparece uno nuevo. Sacarlos obligaria a correr todos
  // los de atras, y los indices que otros guardan (como Enemigo.objetivo)
  // apuntarian a la unidad equivocada.
  Enemigo enemigos[MAX_ENEMIGOS];
  int cant_enemigos;
  float spawn_espera; // segundos hasta el proximo enemigo

  Proyectil proyectiles[MAX_PROYECTILES];

  Cuartel cuarteles[MAX_CUARTELES];
  int cant_cuarteles;

  float preparacion;  // segundos que faltan para la invasion; 0 = ya empezo
  int modo_construir; // 1 = el proximo click izquierdo planta un cuartel

  // La partida va por oleadas: preparacion -> invasion -> preparacion.
  // La invasion termina cuando ya aparecieron todos los de la oleada Y no
  // queda ninguno vivo. Recien ahi vuelve el contador, en 30.
  int oleada;
  int por_aparecer; // enemigos que faltan entrar en la oleada en curso

  Recursos recursos;

  Tesoro tesoros[MAX_TESOROS];
  int cant_tesoros;

  Seleccion seleccion;

  Player player;
  Room rooms[MAX_ROOMS];
} Estado;

#endif // DUNGEON_H
