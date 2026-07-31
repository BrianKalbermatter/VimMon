#ifndef DUNGEON_H
#define DUNGEON_H

// dungeon.h — los tipos de TU juego. Nada de plataforma aca (eso es
// platform.h), nada de cuerpos de funciones (eso es el .c correspondiente).

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
#define MAX_SOLDADOS 12

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
  Terreno terreno;

  Soldado soldados[MAX_SOLDADOS];
  int cant_soldados;

  Seleccion seleccion;

  Player player;
  Room rooms[MAX_ROOMS];
} Estado;

#endif // DUNGEON_H
