#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

float A, B, C;
float cubeWidth = 10;
int width = 160, height = 44;
float zBuffer[160 * 44];
char buffer[160 * 44];
int backgroundASCIICode = ' ';
int distanceFromCam = 60;
float incrementSpeed = 0.6;
float K1 = 40;
float x, y, z;
float ooz;
int idx;
int xp, yp;

float calculateX(int i, int j,
                 int k) { // Esto es una funcion de matriz de 3 dimenciones

  return j * sin(A) * sin(B) * cos(C) - k * cos(A) * sin(B) * cos(C) +
         j * cos(A) * sin(C) + k * sin(A) * sin(C) + i * cos(B) * cos(C);
}

float calculateY(int i, int j, int k) {
  return j * cos(A) * cos(C) + k * sin(A) * cos(C) -
         j * sin(A) * sin(B) * sin(C) - i * cos(B) * sin(C);
}

float calculateZ(int i, int j, int k) {
  return k * cos(A) * cos(B) - j * sin(A) * cos(B) + i * sin(B);
}

void calculateForSurface(float cubeX, float cubeY, float cubeZ, int ch) {
  x = calculateX(cubeX, cubeY, cubeZ);
  y = calculateY(cubeX, cubeY, cubeZ);
  z = calculateZ(cubeX, cubeY, cubeZ) + distanceFromCam;

  ooz = 1 / z;

  xp = (int)(width / 2 + K1 * ooz * x * 2);
  yp = (int)(height / 2 + K1 * ooz * y);
  idx = xp + yp * width;
  if (idx >= 0 && idx < width * height) {
    if (ooz > zBuffer[idx]) {
      zBuffer[idx] = ooz;
      buffer[idx] = ch;
    }
  }
}

int main() {
  printf("\x1b[2J");

  while (1) {
    memset(buffer, backgroundASCIICode, width * height);
    memset(zBuffer, 0, width * height * 4);
    for (float cubeX = -cubeWidth; cubeX < cubeWidth; cubeX += incrementSpeed) {
      for (float cubeY = -cubeWidth; cubeY < cubeWidth;
           cubeY += incrementSpeed) {
        calculateForSurface(cubeX, cubeY, -cubeWidth, '.');
        calculateForSurface(cubeWidth, cubeY, cubeX, '$');

        calculateForSurface(-cubeWidth, cubeY, -cubeX, '~');

        calculateForSurface(cubeX, cubeY, cubeWidth, '#');
        calculateForSurface(cubeX, -cubeWidth, -cubeY, ';');
      }
    }
    printf("\x1b[H");
    for (int k = 0; k < width * height; k++) {
      putchar(k % width ? buffer[k] : 10);
    }
    A += 0.005;
    B += 0.005;
  }
  return 0;
}
/*
 *INCLUDES (líneas 1-4)

  #include <math.h>      // sin(), cos() para rotaciones
  #include <stdio.h>     // printf(), putchar()
  #include <string.h>    // memset()
  #include <unistd.h>    // usleep() (no se usa aquí pero es común)

  ---
  VARIABLES GLOBALES (líneas 7-19)

  float A, B, C;                    // Ángulos de rotación (X, Y, Z)
  float cubeWidth = 10;             // Tamaño del cubo
  int width = 160, height = 44;     // Tamaño de la "pantalla" (columnas x
 filas) float zBuffer[160 * 44];          // Guarda la profundidad de cada pixel
  char buffer[160 * 44];            // Guarda el caracter a mostrar en cada
 pixel int backgroundASCIICode = ' ';    // Caracter de fondo (espacio) int
 distanceFromCam = 60;         // Distancia del cubo a la "cámara" float
 incrementSpeed = 0.6;       // Qué tan detallado se dibuja el cubo float K1 =
 40;                    // Factor de escala para proyección float x, y, z; //
 Coordenadas 3D después de rotar float ooz;                        // "One Over
 Z" = 1/z (para perspectiva) int idx;                          // Índice en el
 buffer int xp, yp;                       // Coordenadas 2D en pantalla

  ¿Qué es zBuffer?
  Guarda qué tan "cerca" está cada pixel de la cámara. Si un punto nuevo está
 más cerca que el anterior, lo reemplaza. Así los objetos cercanos tapan a los
 lejanos.

  ---
  FUNCIONES DE ROTACIÓN 3D (líneas 21-35)

  float calculateX(int i, int j, int k)
  float calculateY(int i, int j, int k)
  float calculateZ(int i, int j, int k)

  Estas funciones aplican matrices de rotación 3D. Toman un punto (i, j, k) y lo
 rotan según los ángulos A, B, C usando trigonometría.

  Es matemática de álgebra lineal - rotar un punto en 3D alrededor de los ejes
 X, Y, Z.

  ---
  calculateForSurface (líneas 37-53)

  void calculateForSurface(float cubeX, float cubeY, float cubeZ, int ch){
      x = calculateX(cubeX, cubeY, cubeZ);    // Rotar el punto en X
      y = calculateY(cubeX, cubeY, cubeZ);    // Rotar el punto en Y
      z = calculateZ(cubeX, cubeY, cubeZ) + distanceFromCam;  // Rotar en Z +
 alejar de cámara

      ooz = 1/z;  // Perspectiva: objetos lejanos se ven más chicos

      // Proyectar 3D → 2D (convertir coordenadas 3D a posición en pantalla)
      xp = (int)(width/2 + K1 * ooz * x * 2);   // Centro + offset
      yp = (int)(height/2 + K1 * ooz * y);

      idx = xp + yp * width;  // Convertir (x,y) a índice lineal del array

      if(idx >= 0 && idx < width * height){     // Si está dentro de la pantalla
          if(ooz > zBuffer[idx]){               // Si este punto está MÁS CERCA
              zBuffer[idx] = ooz;               // Guardar nueva profundidad
              buffer[idx] = ch;                 // Guardar el caracter a mostrar
          }
      }
  }

  ---
  MAIN (líneas 55-75)

  int main(){
      printf("\x1b[2J");  // Código ANSI: limpiar toda la pantalla

      while(1){  // Loop infinito (animación)

  memset() - líneas 59-60

  memset(buffer, backgroundASCIICode, width * height);
  memset(zBuffer, 0, width * height * 4);

  memset(destino, valor, cantidad_bytes) = Llena un bloque de memoria con un
 valor.

  - memset(buffer, ' ', 7040) → Llena todo el buffer con espacios (limpia la
 pantalla)
  - memset(zBuffer, 0, 7040*4) → Llena zBuffer con ceros (resetea
 profundidades). El *4 es porque cada float ocupa 4 bytes.

  Dibujar el cubo - líneas 61-65

  for(float cubeX = -cubeWidth; cubeX < cubeWidth; cubeX += incrementSpeed){
      for(float cubeY = -cubeWidth; cubeY < cubeWidth; cubeY += incrementSpeed){
          calculateForSurface(cubeX, cubeY, -cubeWidth, '$');
      }
  }

  Recorre todos los puntos de una cara del cubo (la cara frontal donde Z =
 -cubeWidth) y dibuja cada punto.

  Nota: Solo dibuja UNA cara. Para un cubo completo necesitas 6 llamadas a
 calculateForSurface con diferentes caras.

  Mostrar en pantalla - líneas 66-70

  printf("\x1b[H");  // Código ANSI: mover cursor a posición (0,0)

  for (int k = 0; k < width * height; k++){
      putchar(k % width ? buffer[k] : 10);  // 10 = '\n' (nueva línea)
  }

  - k % width ? buffer[k] : 10 → Si NO es fin de fila, imprime el caracter. Si
 ES fin de fila, imprime salto de línea.

  Rotar - líneas 71-72

  A += 0.005;  // Incrementar ángulo de rotación en X
  B += 0.005;  // Incrementar ángulo de rotación en Y

  Esto hace que el cubo rote un poquito en cada frame.

  ---
  RESUMEN VISUAL

  ┌─────────────────────────────────────┐
  │  1. Limpiar buffers (memset)        │
  │  2. Para cada punto del cubo:       │
  │     - Rotarlo en 3D                 │
  │     - Proyectarlo a 2D              │
  │     - Guardarlo si está más cerca   │
  │  3. Imprimir el buffer              │
  │  4. Rotar un poco más               │
  │  5. Repetir                         │
  └─────────────────────────────────────┘
 *
 *
 *
 *
 *
 *
 *
 *
 *
 * */
