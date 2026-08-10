/*
 * Expresion Condicional Operador '?'
 * Sintaxis
 * Condicion ? Expresion 1: Expresion 2
 * */

#include <stdio.h>

//Diferencia entre puts() y prinf(). Los dos imprimen algo pero el puts solo strings no puede concatenar otra valor.
int main(){
    int num;
    puts("Escribe un numero:\n");
    scanf("%i",&num);

    (num%2==0) ? puts("El numero es par\n") : puts("Es impar\n");
    return 0;
}
/*
Ejercicio: Calculadora de tarifas de taxi

  Una empresa de taxis cobra según las siguientes reglas:

  1. Tarifa base: $50 por subir al taxi
  2. Por distancia:
    - Primeros 5 km: $10 por km
    - De 5 a 15 km: $8 por km
    - Más de 15 km: $5 por km
  3. Recargos:
    - Si es horario nocturno (entre 22:00 y 06:00): +30% sobre el total
    - Si es fin de semana (sábado o domingo): +20% sobre el total
    - Si es feriado: +50% sobre el total
    - Los recargos son acumulables (puede ser noche de feriado en fin de semana)
  4. Descuentos:
    - Si el pasajero es mayor de 65 años: -15%
    - Si tiene tarjeta de la empresa: -10%
    - Los descuentos se aplican después de los recargos
    - Los descuentos también son acumulables
  5. Restricción: El monto mínimo a pagar es siempre $50, sin importar los descuentos.

  Entrada: Distancia en km, hora del viaje (0-23), día de la semana (1-7), si es feriado (s/n), edad del pasajero, si tiene tarjeta (s/n).

  Salida: Desglose del cálculo y monto final a pagar.
*/
