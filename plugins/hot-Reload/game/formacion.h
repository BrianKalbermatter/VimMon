#ifndef FORMACION_H
#define FORMACION_H

// formacion.h — donde se para cada unidad de un grupo.
//
// Sin esto, ordenarle a diez soldados que vayan al mismo punto los apila a los
// diez en la misma celda y ves uno solo. En Age of Empires cada unidad recibe
// su propio lugar dentro de una rejilla, y por eso el grupo llega formado.

// Calcula donde tiene que pararse la unidad numero `indice` de un grupo de
// `total`, si el grupo se centra en (cx, cy). Escribe el resultado en x/y.
//
// El indice va de 0 a total-1. Con total == 1 devuelve (cx, cy) tal cual.
void formacion_slot(int indice, int total, float cx, float cy, float *x,
                    float *y);

#endif // FORMACION_H
