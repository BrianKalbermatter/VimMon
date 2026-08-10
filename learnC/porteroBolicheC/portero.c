/*El laburo: "El portero del boliche" 🎟️

Sos el que tiene que procesar los ingresos de un evento. Te pasan un archivo de texto, ingresos.txt, donde cada línea es un registro de una persona que entró:

// dni;nombre;edad;hora_ingreso
// 40123456;Pereyra;22;23:15
// 38999111;Gomez;31;00:42
// 40123456;Pereyra;22;01:30
// 41555222;Lopez;17;23:50
// 38999111;Gomez;31;23:05

Campos separados por ;. La hora es HH:MM formato 24h (ojo, después de medianoche sigue el mismo evento). El archivo puede tener desde 5 líneas hasta 50.000.

Lo que tu programa en C tiene que hacer (obligatorio):

1. Leer el archivo y cargar cada registro en una struct (nada de procesar línea suelta tirada por ahí — modelá el dato).
2. Reportar cuánta gente entró en total y cuántas PERSONAS distintas (por DNI, porque uno puede entrar, salir y volver a entrar — fijate que 40123456 aparece dos veces).
3. Detectar y listar a los menores de 18 que entraron (acá hay un Lopez de 17 que no tendría que estar). Mostrá DNI y nombre.
4. Decir en qué FRANJA horaria entró más gente. Dividí la noche en bloques de una hora (23:00–23:59, 00:00–00:59, 01:00–01:59, etc.) y decime cuál fue el pico.

Reglas del juego:

- C puro. stdio.h, stdlib.h, string.h y nada más. Sin librerías mágicas.
- Tenés que manejar el archivo vos: abrirlo, leerlo, cerrarlo (si te olvidás el fclose, te mato 😄).
- Pensá la estructura de datos ANTES de programar. ¿Array fijo? ¿Memoria dinámica? ¿Cómo contás DNIs únicos sin recorrer todo mil veces? Ahí está el verdadero ejercicio.

Si te sobra hambre (bonus, opcional):

- Nivel 2: que el programa funcione aunque no sepas cuántas líneas tiene el archivo de antemano (malloc/realloc, la posta).
- Nivel 3: ordená el reporte de menores por edad descendente. Implementá VOS el ordenamiento, no qsort.

---
Una sola condición, y es la importante: no me pidas la solución. Cocinalo vos. Trabate, puteá, resolvé. Ese sentir de "lo saqué yo" es justo lo que viniste a buscar.
*/

#include <stdint.h>
#include <stdio.h>

// Lo que tengo que hacer es
// 1. Abrir el archivo de ingresos de un evento
// 2. Cargarlo en un struct(REGISTRO)
// Con este struct le digo a C que voy a querer un registro de todas las personas que ingresan
// typedef struct{
//    char dni[9];
//    char nombre[50];
// }
//
int main(){
  typedef struct {
    int dni[9];
    int edad;
    char nombre[50];
  } portero;

  portero portero;

  FILE *arch; // Archivo vacio
  arch = fopen("personas.txt", "r"); // abro el archivo en modo lectura

  char personas[50];


  int contadorPersonas = 0;
  if (arch == NULL){
    printf("No se puede abrir el archivo\n");
    return 1; // Para que de error!
  
    contadorPersonas++;
  }
  if (portero.edad < "17"){
    printf("Es menor de edad");
  }
  return 0;
}
