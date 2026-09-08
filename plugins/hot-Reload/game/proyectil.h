#ifndef PROYECTIL_H
#define PROYECTIL_H

// proyectil.h — las balas que van por el aire.
//
// Un proyectil no sabe a quien le pega: solo viaja. Quien resuelve los impactos
// es combate.c, que es el unico que conoce a los dos bandos. Asi este archivo
// no depende de soldados ni de enemigos.

#define BANDO_SOLDADO 0
#define BANDO_ENEMIGO 1

typedef struct {
  float x, y;
  float dx, dy;    // direccion normalizada: largo 1, pura direccion
  float velocidad; // celdas por segundo
  int danio;
  int bando; // quien disparo; una bala no lastima al que la tiro
  int activo;
} Proyectil;

// Busca una ranura libre en el arreglo y larga una bala desde (x, y) hacia
// (ox, oy). Devuelve 1 si salio, 0 si no habia lugar.
//
// Se apunta a donde el objetivo esta AHORA, no a donde va a estar: si la
// victima camina, la bala le pasa por atras. Es a proposito, y es lo que hace
// que moverse sirva de algo.
int proyectil_disparar(Proyectil *ps, int cant, float x, float y, float ox,
                       float oy, int danio, int bando);

// Adelanta todas las balas activas y apaga las que se fueron del rectangulo.
// Sin ese apagado, una bala perdida viaja para siempre ocupando su ranura.
void proyectiles_update(Proyectil *ps, int cant, float dt, int x0, int y0,
                        int x1, int y1);

#endif // PROYECTIL_H
