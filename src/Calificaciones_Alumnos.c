/*
 *Un alumnno desea saber cual sera su calificacion final en la materia de Algoritmos.
 *Dicha calificacion se compone de los siguientes porcentajes:
 *55% del promedio de sus tres calificaciones parciales.
 *30% de la calificacion del examen final.
 *15% de la calificacion de un trabajo final.
 */

#include <stdio.h>

int main(){
    float promedio_cali_tres_par;
    float p1, p2, p3;
    float examen_final_cali;
    float trabajo_final_cali;
    float promedio_total;
    float porcentajeParciales;
    float examenFinal;
    float trabajoFinal;

    //Calificacion Parciales:
    printf("Escribe las calificaciones de los examenes parciales:\n");
    scanf("%f", &p1); 
    scanf("%f", &p2); 
    scanf("%f", &p3);
    promedio_cali_tres_par = (p1 + p2 + p3) / 3;
    printf("\nEl primedio de las 3 calificaciones parciales es:\n");
    printf("%f", promedio_cali_tres_par);
    
    //Examen Final:
    printf("\nEscribe la calificacion del examen final:\n");
    scanf("%f", &examen_final_cali);
    
    //Trabajo Integrador Final:
    printf("Escribe la calificacion del trabajo final:\n");
    printf("La calificacion del trabajo final es:\n");
    scanf("%f", &trabajo_final_cali);

    printf("\nEl porcentaje de todas las calificaciones es de:\n");
    porcentajeParciales = promedio_cali_tres_par * 0.55;
    examenFinal = examen_final_cali * 0.30;
    trabajoFinal = trabajo_final_cali * 0.15;
    promedio_total = porcentajeParciales + examenFinal + trabajoFinal;
    
    printf("%f", promedio_total);
    return 0;
}
