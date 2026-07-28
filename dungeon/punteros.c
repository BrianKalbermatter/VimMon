// punteros.c — laboratorio de punteros.
//
// Compilar y correr:
//     gcc -Wall -g punteros.c -o punteros && ./punteros
//
// Leelo de arriba a abajo. Cada parte se apoya en la anterior.
// Al final hay ejercicios para completar.

#include <stdio.h>
#include <string.h>

// ============================================================
// PARTE 1 — Toda variable vive en una direccion
// ============================================================
//
// Una variable es una caja en memoria. La caja tiene dos cosas:
//   - un VALOR (lo que guarda)
//   - una DIRECCION (donde esta ubicada)
//
// Un puntero es una variable que guarda una DIRECCION.
// Nada mas que eso. No es magia, no es un tipo especial de dato.

void parte1_direcciones(void) {
  printf("\n=== PARTE 1: direcciones ===\n");

  int hp = 100;

  // &hp  -> "la direccion de hp"
  // int* -> "esta variable guarda la direccion de un int"
  int *ptr = &hp;

  printf("hp vale        : %d\n", hp);
  printf("hp vive en     : %p\n", (void *)&hp);
  printf("ptr guarda     : %p\n", (void *)ptr);

  // *ptr -> "el valor que hay EN esa direccion". Se llama DESREFERENCIAR.
  printf("*ptr vale      : %d\n", *ptr);

  // Como ptr apunta a hp, escribir por ptr modifica hp.
  // No hay copia de por medio: es la misma caja.
  *ptr = 75;
  printf("despues de *ptr = 75, hp vale: %d\n", hp);

  // Regla para leer el simbolo *:
  //   en la DECLARACION  int *ptr;  -> "ptr es un puntero a int"
  //   en el USO          *ptr = 75; -> "el valor apuntado por ptr"
  // Mismo simbolo, dos significados distintos. Eso confunde a todo el mundo.
}

// ============================================================
// PARTE 2 — C pasa TODO por copia. Este es el nucleo del asunto.
// ============================================================
//
// Cuando llamas a una funcion, los parametros son COPIAS del valor.
// La funcion trabaja sobre la copia y la copia se muere al terminar.
// Por eso una funcion no puede modificar tus variables... salvo que le
// pases la DIRECCION.

void curar_por_copia(int vida) {
  vida = vida + 30; // modifica la copia local, no el original
}

void curar_por_puntero(int *vida) {
  *vida = *vida + 30; // va a la direccion y modifica el original
}

void parte2_copia_vs_puntero(void) {
  printf("\n=== PARTE 2: copia vs puntero ===\n");

  int hp = 50;

  curar_por_copia(hp);
  printf("despues de curar_por_copia   : %d  <- no cambio nada\n", hp);

  curar_por_puntero(&hp);
  printf("despues de curar_por_puntero : %d  <- ahora si\n", hp);

  // ESTO es lo que rompia tu move_player en dungeon.c.
  // La variable playing vive dentro de main. move_player no la ve.
  // Para que move_player pueda terminar el juego, necesita: int *playing
}

// ============================================================
// PARTE 3 — Structs: el operador ->
// ============================================================

typedef struct {
  char name[32];
  int hp;
  int stamina;
} Character;

// Recibir el struct por copia significa copiar los 40+ bytes enteros
// en cada llamada, y los cambios se pierden al volver.
void danio_por_copia(Character c) { c.hp -= 20; }

// Recibir un puntero copia solo la direccion (8 bytes) y trabaja
// sobre el original. Por eso casi todas las funciones de C que tocan
// structs reciben punteros.
void danio_por_puntero(Character *c) {
  // (*c).hp es correcto pero incomodo de escribir.
  // c->hp significa exactamente lo mismo. Se lee "el hp de lo que apunta c".
  c->hp -= 20;
}

void parte3_structs(void) {
  printf("\n=== PARTE 3: structs ===\n");

  Character hero;
  strcpy(hero.name, "Hero");
  hero.hp = 100;
  hero.stamina = 50;

  danio_por_copia(hero);
  printf("%s hp tras danio_por_copia   : %d\n", hero.name, hero.hp);

  danio_por_puntero(&hero);
  printf("%s hp tras danio_por_puntero : %d\n", hero.name, hero.hp);

  // Resumen de sintaxis:
  //   hero.hp   -> tengo el struct
  //   c->hp     -> tengo un puntero al struct
}

// ============================================================
// PARTE 4 — Arrays: el nombre YA es una direccion
// ============================================================

void bajar_vida_a_todos(Character *equipo, int n) {
  for (int i = 0; i < n; i++) {
    equipo[i].hp -= 10; // indexar un puntero funciona igual que un array
  }
}

void parte4_arrays(void) {
  printf("\n=== PARTE 4: arrays ===\n");

  Character equipo[3];
  strcpy(equipo[0].name, "Guerrero");
  strcpy(equipo[1].name, "Mago");
  strcpy(equipo[2].name, "Arquero");
  for (int i = 0; i < 3; i++)
    equipo[i].hp = 100;

  // Sin &: el nombre de un array ya ES la direccion de su primer elemento.
  bajar_vida_a_todos(equipo, 3);

  for (int i = 0; i < 3; i++)
    printf("%-10s hp: %d\n", equipo[i].name, equipo[i].hp);

  // Por eso un array pasado a una funcion SI se modifica, y un int no.
  // Nunca fue una excepcion a la regla: es la regla, aplicada a una direccion.
}

// ============================================================
// EJERCICIOS — completa el cuerpo de cada funcion
// ============================================================

// EJERCICIO 1
// Hace que intercambie de verdad los dos valores.
// Pista: necesitas una variable temporal.
void intercambiar(int *a, int *b) {
  // TODO
  (void)a;
  (void)b;
}

// EJERCICIO 2
// Debe restar el danio al hp del objetivo, sin bajar de 0,
// y devolver 1 si murio, 0 si sigue vivo.
int aplicar_danio(Character *objetivo, int danio) {
  // TODO
  (void)objetivo;
  (void)danio;
  return 0;
}

// EJERCICIO 3
// Debe devolver un puntero al personaje del array con menos hp.
// Devolver NULL si n <= 0.
Character *mas_debil(Character *equipo, int n) {
  // TODO
  (void)equipo;
  (void)n;
  return NULL;
}

void ejercicios(void) {
  printf("\n=== EJERCICIOS ===\n");

  int x = 10, y = 20;
  intercambiar(&x, &y);
  printf("ej1  x=%d y=%d      (esperado: x=20 y=10)\n", x, y);

  Character orco;
  strcpy(orco.name, "Orco");
  orco.hp = 15;
  int murio = aplicar_danio(&orco, 40);
  printf("ej2  hp=%d murio=%d (esperado: hp=0 murio=1)\n", orco.hp, murio);

  Character equipo[3];
  strcpy(equipo[0].name, "Guerrero");
  equipo[0].hp = 80;
  strcpy(equipo[1].name, "Mago");
  equipo[1].hp = 35;
  strcpy(equipo[2].name, "Arquero");
  equipo[2].hp = 60;

  Character *debil = mas_debil(equipo, 3);
  printf("ej3  %s            (esperado: Mago)\n",
         debil ? debil->name : "NULL");
}

int main(void) {
  parte1_direcciones();
  parte2_copia_vs_puntero();
  parte3_structs();
  parte4_arrays();
  ejercicios();
  return 0;
}
