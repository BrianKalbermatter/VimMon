#ifndef VIMMON_MATH3D_H
#define VIMMON_MATH3D_H

#include "renderer3d.h"

// ============================================================
// La matematica minima para 3D. NADA mas que lo que se usa.
//
// Una Mat4 es una matriz de 4x4 guardada en UN array plano de 16 floats,
// en orden POR COLUMNAS (column-major), que es lo que espera GLSL.
// m[0..3] es la primera columna, m[4..7] la segunda, y asi.
// ============================================================

typedef struct { float m[16]; } Mat4;

V3   v3(float x, float y, float z);
V3   v3_sub(V3 a, V3 b);
V3   v3_add(V3 a, V3 b);
V3   v3_escalar(V3 a, float k);
V3   v3_cruz(V3 a, V3 b);      // producto vectorial: da el perpendicular
float v3_punto(V3 a, V3 b);    // producto escalar
V3   v3_normal(V3 a);          // mismo sentido, largo 1

Mat4 mat4_identidad(void);
Mat4 mat4_mul(Mat4 a, Mat4 b);

// Modelo: escala -> rota (X, luego Y, luego Z) -> traslada. En ese orden,
// que es el unico que hace lo que uno espera al leerlo.
Mat4 mat4_modelo(V3 pos, V3 rot_grados, V3 escala);

// Vista: la camara. Mueve el mundo entero para que la camara quede en el
// origen mirando hacia -Z. La camara nunca se mueve; se mueve todo lo demas.
Mat4 mat4_mirar(V3 ojo, V3 destino, V3 arriba);

// Proyeccion en perspectiva. Aca es donde lo lejano se achica, y pasa
// porque la matriz mete la profundidad en la componente w: la GPU despues
// divide x, y y z por w. Esa division ES la perspectiva.
Mat4 mat4_perspectiva(float fov_grados, float aspecto, float cerca, float lejos);

#endif // VIMMON_MATH3D_H
