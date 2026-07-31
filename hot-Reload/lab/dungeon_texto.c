#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "platform.h"

// <--- Constantes --->

#define MAX_ITEMS 50
#define MAX_ROOMS 8
#define MAX_DESC 200 // Maxima descripcion [MAX_DESC]

// Estructura de Datos
typedef struct {
  char nombre[50];
  char descripcion[MAX_DESC];
  int heal_amount; // Potion
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
  int norte, sur, este, oeste; // -1 la habitacion no existe
  Items items[MAX_ITEMS];
  int item_cont;
  int enemigo_hp;
  int enemigo_nombre[50];
  int enemigo_ataque;
  int limpieza; // 1 si un enemigo muere
} Room;

typedef struct{
  
}Estado;
// Concepto importante:
//      typedef struct es un alias al que se le coloca primero como un prototipo
//      de funcion y luego se la puede usar normalmente en cualquier lado, no es
//      lo mismo que struct nombre porque: eso significa que ya la tengo que
//      usar en ese mismo lugar en donde esta, en cambio aca no, la puedo usar
//      cuando quiera.

// --- Global Game State ---

Player player;
Room rooms[MAX_ROOMS];

// --- Function Prototypes ---
// Inicio de game
void init_game();
// Iniciar todo sobre rooms hasta las pantallas
void init_rooms();
void printRooms();

// Displays
void display_welcome();
void display_stats();
void display_room();
void display_inventario();

// Movimiento del jugador
void move_player(int direction);

// Items
void pickup_item();

// Enemigos
void ataque_enemigo();
void chequeo_muerte();
void chequeo_victorias();
void clear_input_buffer();
int get_input();

int main() {
  srand(time(NULL)); // Basicamente es para aleatoriedad random y que la
                     // funcion, srand(stdlib.h) no suponga nada. Pero el tiempo aleatoriamente solamente: o sea que es NULL entonces no es cualquier tiempo.
  // Implementar que se pueda mover en tiempo real el personaje e ir por frame,
  // actualizandose.
  printRooms();
  init_game();
  int playing = 1;
  while (playing) {
    display_room();
    display_stats();
 

    int choice = get_input(); // <- Este va a ser una funcion de que tiene que
                              // colocar algo el usuario.
    switch (choice) {
    case 1:
      move_player(0); // W
      break;          // North
    case 2:
      move_player(1); // S
      break;          // South
    case 3:
      move_player(2); // A
      break;          // East
    case 4:
      move_player(3); // D
      break;          // West
    case 5:
      pickup_item();
      break;
    case 6:
      // use_item();
      break;
    case 7:
      display_inventario();
      break;
    case 8:
      ataque_enemigo();
      break;
    case 9:
      printf(YELLOW "\n Gracias por Jugar! \n" RESET);
      playing =
          0; // Resetea al jugador valiendo cero de nuevo, entonces puede salir!
      break;
    default:
      printf(RED "\n Opcion Invalida! \n" RESET);
    }
    chequeo_muerte();
    chequeo_victorias();

    if (player.hp <= 0) {
      playing = 0; // Juegador se murio
    }
  }
  return 0;
}

void init_game() {
  // INICIALIZAR JUGADOR
  strcpy(player.name, "Hero"); // strcpy(Destino, Origen);
  player.hp = 100;
  player.max_hp = 100;
  player.attack = 15;
  player.defense = 5;
  player.gold = 20;
  player.level = 1;
  player.experiense = 0;
  player.habitacion_actual = 0;

  // Give player a starting potion
  strcpy(player.inventario[0].nombre, "Poscion de Salud");
  strcpy(player.inventario[0].descripcion, "Restaura la vida un 30 de HP");
  player.inventario[0].heal_amount = 30;
  player.inventario[0].damage_bonus = 0;
  player.cont_inventario =
      1; // Para que quiero contar el inventario aca? para mi no hace falta

  init_rooms();
}
void display_room(){
  printRooms();
}
void printRooms() {
  char dx = 40;
  char dy = 40;
  int i;
  int n;
  for (i=0;n == dx; i++)
  {
    printf("#");
  }
}

void init_rooms() {
  // Room 0: Entrando al Hall
  printf("----------------------------------------------------------\n");
  strcpy(rooms[0].name, "Entrando al Hall");
  printf("----------------------------------------------------------\n");
  printRooms();
}
/*
void move_player(int direction) {

  // Estaba pensando en colocarlo en un switch cases
  int dx = 0;
  int dy = 0;
  while (playing != 0) { // Esto quiero decir que si el jugador no esta muerto
                         // entonces pasa: Jugador se mueve
    if (player == move_player(0)) {
      // El jugador se puede mover con W
    } else if (player == move_player(1)) {
      // El jugador se puede mover con S
    }
  }
}
*/
