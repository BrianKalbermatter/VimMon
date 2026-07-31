#include "formacion.h"

// Separacion entre unidades. En X es el doble que en Y a proposito: una celda
// de terminal es como el doble de alta que de ancha, asi que con el mismo
// numero en los dos ejes la formacion se ve aplastada.
#define ESPACIO_X 2
#define ESPACIO_Y 1

// Cuantas columnas tiene la rejilla: la primera cuyo cuadrado alcanza para
// todos. Con 9 unidades da 3, con 10 da 4. Es la forma mas cuadrada posible sin
// usar sqrt.
static int columnas_para(int total) {
  int c = 1;
  while (c * c < total)
    c++;
  return c;
}

void formacion_slot(int indice, int total, float cx, float cy, float *x,
                    float *y) {
  if (total <= 1) {
    *x = cx;
    *y = cy;
    return;
  }

  int columnas = columnas_para(total);
  // Division que redondea para arriba: 10 unidades en 4 columnas son 3 filas,
  // no 2. La ultima fila queda incompleta y esta bien.
  int filas = (total + columnas - 1) / columnas;

  int col = indice % columnas;
  int fila = indice / columnas;

  // Restar el centro de la rejilla deja la formacion centrada en (cx, cy) en
  // vez de colgando hacia abajo y a la derecha. Se usa /2.0f y no /2 porque con
  // un numero par de columnas el centro cae entre dos, y esa mitad importa.
  *x = cx + ((float)col - (columnas - 1) / 2.0f) * ESPACIO_X;
  *y = cy + ((float)fila - (filas - 1) / 2.0f) * ESPACIO_Y;
}
