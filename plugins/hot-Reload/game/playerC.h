#ifndef PLAYERC_H
#define PLAYERC_H

// playerC.h — el CONTRATO del soldado. Aca solo se declara que existe.
// El cuerpo de cada funcion vive en playerC.c: si lo pusieras aca, cada .c que
// incluya este header se quedaria con su propia copia y el linker te tiraria
// "multiple definition".

// Un soldado al estilo Age of Empires: no salta de casillero en casillero, sino
// que camina hacia un destino a lo largo de varios frames.
//
// Por que float y no int: si la posicion fuera entera, con una velocidad de 8
// celdas por segundo a 120 fps cada frame avanzaria 0.066 celdas, que redondeado
// a int es 0. El soldado no se moveria NUNCA. Los decimales se acumulan frame a
// frame y recien al dibujar se recortan a entero.
typedef struct {
  float x, y; // posicion actual, con decimales
  float destino_x, destino_y;
  int moviendose;   // 0 = quieto, ya llego (o nunca le ordenaron nada)
  float velocidad;  // celdas por segundo
  int seleccionado; // 1 si entro en la ultima seleccion por area

  // --- Combate ---
  int hp, max_hp;
  int vivo;       // 0 = muerto: deja de dibujarse y de ser un objetivo valido
  int ataque;     // dania por golpe
  float cadencia; // segundos entre golpe y golpe
  float espera;   // cuenta regresiva hasta poder golpear de nuevo

  // Hasta donde llega su tiro. Es un CAMPO y no una constante porque cada tipo
  // de unidad alcanza distinto: el espadachin tiene que pegarse encima y el
  // fusilero le tira desde mas lejos de lo que el enemigo puede contestar.
  float alcance;

  // Que clase de unidad es. El Soldado no sabe que significa cada numero: la
  // tabla que le da sentido es cosa del juego (UNIDAD en dungeon.h), no de la
  // mecanica de caminar y disparar que vive en este archivo.
  int tipo;
} Soldado;

// Lo planta en (x, y) quieto y sin destino pendiente.
void soldado_init(Soldado *s, float x, float y);

// Le marca un destino. Equivale a hacer click en el piso en AoE: no lo
// teletransporta, solo anota adonde tiene que ir.
void soldado_ordenar_ir(Soldado *s, int x, int y);

// Lo acerca al destino lo que corresponda segun dt. Se llama una vez por frame.
void soldado_update(Soldado *s, float dt);

// --- Teclado (disponible, hoy sin usar) --------------------------------------
// Traduce WASD a un desplazamiento de una celda. Devuelve 1 si la tecla era de
// movimiento y dejo algo en dx/dy, o 0 si no le corresponde.
//
// Hoy el soldado se maneja solo con el mouse, asi que nadie la llama. Se deja
// compilada dentro de game.so a proposito: el dia que quieras mover una unidad
// con teclas, ya esta escrita y probada, y entra en caliente sin reiniciar.
int tecla_a_direccion(char tecla, int *dx, int *dy);

#endif // PLAYERC_H
