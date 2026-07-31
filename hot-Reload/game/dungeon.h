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
#define MUNDO_ANCHO 400
#define MUNDO_ALTO 140

#define MAX_ENEMIGOS 24
#define MAX_CUARTELES 4
#define MAX_PROYECTILES 96

// --- Poblacion ---------------------------------------------------------------
//
// Con cuantos lugares arrancas y cuantos suma cada casa. El techo YA NO ES UNA
// CONSTANTE: vive en Recursos.poblacion_max y sube cada vez que plantas una.
#define POBLACION_INICIAL 10
#define CASA_POBLACION 5

#define MAX_CASAS 48

// El tope de soldados dejo de ser una regla del juego y paso a ser lo que
// siempre fue de verdad: el tamanio del arreglo, o sea una decision de MEMORIA.
//
// En C no existe "sin limite" mientras el arreglo sea de tamanio fijo: el host
// te presta un bloque de 16MB y ahi entra todo. Lo que si se puede hacer es que
// el techo del arreglo NO PUEDA quedarse corto, derivandolo de la poblacion
// maxima alcanzable. Si maniana subis MAX_CASAS o CASA_POBLACION, el arreglo
// crece con ellos y no hay forma de desincronizarlos.
//
// Quien limita la tropa ahora es la poblacion, que es lo que vos controlas
// plantando casas. A 52 bytes por soldado, 250 son 13KB: nada.
#define MAX_SOLDADOS (POBLACION_INICIAL + MAX_CASAS * CASA_POBLACION)

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

// Cuantas celdas corre la camara por cada tecla de WASD. La vertical es la
// mitad porque una celda de terminal es como el doble de alta que ancha: con el
// mismo numero, subir y bajar se sentiria el doble de rapido que ir al costado.
#define CAMARA_PASO_X 4
#define CAMARA_PASO_Y 2

// --- Tipos de unidad ---------------------------------------------------------
//
// Mismo truco que TipoRecurso: el enum ordena y UNIDAD_CANTIDAD queda valiendo
// solo lo que hay. La tabla UNIDAD de abajo tiene una fila por cada uno, asi que
// agregar un tipo es agregar el enum y su fila; no hay ningun switch que
// actualizar ni ningun if por tipo desparramado por el codigo.
typedef enum {
  UNIDAD_ESPADACHIN,
  UNIDAD_ARQUERO,
  UNIDAD_FUSILERO,
  UNIDAD_CANTIDAD // no es un tipo: es cuantos hay
} TipoUnidad;

// La ficha de cada tipo. Todo lo que diferencia a un espadachin de un fusilero
// esta ACA ADENTRO y en ningun otro lado: si el balance se siente mal, se toca
// esta tabla y nada mas.
typedef struct {
  const char *nombre;
  const char *corto;  // etiqueta de 3 letras para el panel
  char ch;            // como se dibuja en el mapa
  unsigned char color; // solo para el panel: en el mapa manda el color de vida
  int hp;
  int ataque;
  float cadencia;  // segundos entre tiro y tiro
  float alcance;   // celdas
  float velocidad; // celdas por segundo
} UnidadDef;

// Se declara aca y se define en dungeon.c, igual que RECURSO_NOMBRE: si la
// tabla viviera en el header, cada .c que lo incluya tendria su propia copia.
extern const UnidadDef UNIDAD[UNIDAD_CANTIDAD];

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

typedef struct {
  int cantidad[REC_CANTIDAD]; // todos arrancan en 0
  int poblacion;              // unidades vivas ocupando lugar

  // Techo de poblacion. Arranca en POBLACION_INICIAL y sube CASA_POBLACION por
  // cada casa. Ningun soldado se entrena si pasarlo lo haria superar.
  int poblacion_max;
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

// El cuartel ya NO entrena solo: es el lugar desde donde salen las unidades que
// vos pedis con 1/2/3. Por eso perdio el temporizador y le queda solo la cuenta
// de los que saco, que sirve para no apilarlos todos en la misma celda.
typedef struct {
  int x, y;
  int activo;
  int entrenados;
} Cuartel;

// La casa no hace nada por si misma: existe para SUBIR EL TECHO de poblacion.
// Por eso no tiene temporizador ni estado propio, solo donde esta parada.
typedef struct {
  int x, y;
  int activo;
} Casa;

// Que planta el proximo click izquierdo. Antes esto era un int 0/1 que solo
// sabia decir "cuartel si o no"; con dos edificios ya no alcanzaba, y un
// segundo flag suelto habilitaria el estado imposible de tener los dos
// prendidos a la vez. El enum hace que eso no se pueda ni escribir.
typedef enum {
  CONSTRUIR_NADA,
  CONSTRUIR_CUARTEL,
  CONSTRUIR_CASA
} ModoConstruir;

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
  int piso_alto;  // primera fila del panel: el mapa va de 0 a piso_alto-1
  int panel_alto; // filas del panel; sale del minimapa, no de una constante

  // Esquina superior izquierda del mundo que se esta viendo. La ventana mide
  // ancho x piso_alto.
  int cam_x, cam_y;

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

  Casa casas[MAX_CASAS];
  int cant_casas;

  float preparacion;  // segundos que faltan para la invasion; 0 = ya empezo
  ModoConstruir modo_construir; // que planta el proximo click izquierdo

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
