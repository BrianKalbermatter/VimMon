#include "math3d.h"
#include <math.h>

#define GRADOS_A_RAD  0.017453292519943295f   // pi / 180

V3 v3(float x, float y, float z) { return (V3){ x, y, z }; }
V3 v3_sub(V3 a, V3 b) { return (V3){ a.x - b.x, a.y - b.y, a.z - b.z }; }
V3 v3_add(V3 a, V3 b) { return (V3){ a.x + b.x, a.y + b.y, a.z + b.z }; }
V3 v3_escalar(V3 a, float k) { return (V3){ a.x * k, a.y * k, a.z * k }; }

float v3_punto(V3 a, V3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

// El producto vectorial da un vector PERPENDICULAR a los dos. Con eso se
// saca el "derecha" de la camara a partir de "hacia donde mira" y "arriba".
V3 v3_cruz(V3 a, V3 b)
{
    return (V3){ a.y*b.z - a.z*b.y,
                 a.z*b.x - a.x*b.z,
                 a.x*b.y - a.y*b.x };
}

V3 v3_normal(V3 a)
{
    float largo = sqrtf(v3_punto(a, a));
    // Guard clause: dividir por cero da infinito y contamina toda la escena.
    if (largo < 1e-6f) return (V3){ 0.0f, 0.0f, 0.0f };
    return v3_escalar(a, 1.0f / largo);
}

Mat4 mat4_identidad(void)
{
    Mat4 r = {{ 0 }};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

// Multiplicar matrices NO es conmutativo: a*b != b*a. El orden es el orden
// en que se aplican las transformaciones, y equivocarlo es EL bug clasico
// de 3D: el objeto orbita el origen en vez de girar sobre si mismo.
Mat4 mat4_mul(Mat4 a, Mat4 b)
{
    Mat4 r = {{ 0 }};
    for (int col = 0; col < 4; col++)
        for (int fila = 0; fila < 4; fila++) {
            float suma = 0.0f;
            for (int k = 0; k < 4; k++)
                suma += a.m[k * 4 + fila] * b.m[col * 4 + k];
            r.m[col * 4 + fila] = suma;
        }
    return r;
}

Mat4 mat4_modelo(V3 pos, V3 rot_grados, V3 escala)
{
    float rx = rot_grados.x * GRADOS_A_RAD;
    float ry = rot_grados.y * GRADOS_A_RAD;
    float rz = rot_grados.z * GRADOS_A_RAD;
    float sx = sinf(rx), cx = cosf(rx);
    float sy = sinf(ry), cy = cosf(ry);
    float sz = sinf(rz), cz = cosf(rz);

    // Rotacion combinada Z*Y*X, ya multiplicada a mano para no encadenar
    // tres mat4_mul en algo que se llama una vez por objeto por frame.
    float r00 =  cy*cz,             r01 =  cy*sz,             r02 = -sy;
    float r10 =  sx*sy*cz - cx*sz,  r11 =  sx*sy*sz + cx*cz,  r12 =  sx*cy;
    float r20 =  cx*sy*cz + sx*sz,  r21 =  cx*sy*sz - sx*cz,  r22 =  cx*cy;

    Mat4 r = {{ 0 }};
    // Cada columna de rotacion se multiplica por su escala.
    r.m[0]  = r00 * escala.x;  r.m[1]  = r01 * escala.x;  r.m[2]  = r02 * escala.x;
    r.m[4]  = r10 * escala.y;  r.m[5]  = r11 * escala.y;  r.m[6]  = r12 * escala.y;
    r.m[8]  = r20 * escala.z;  r.m[9]  = r21 * escala.z;  r.m[10] = r22 * escala.z;
    // La ultima columna es la traslacion. Por eso una matriz 4x4 y no 3x3:
    // la cuarta fila/columna existe para poder MOVER, no solo rotar y escalar.
    r.m[12] = pos.x;  r.m[13] = pos.y;  r.m[14] = pos.z;  r.m[15] = 1.0f;
    return r;
}

Mat4 mat4_mirar(V3 ojo, V3 destino, V3 arriba)
{
    V3 f = v3_normal(v3_sub(destino, ojo));   // adelante
    V3 s = v3_normal(v3_cruz(f, arriba));     // derecha
    V3 u = v3_cruz(s, f);                     // arriba real (ya perpendicular)

    Mat4 r = mat4_identidad();
    r.m[0] = s.x;  r.m[4] = s.y;  r.m[8]  = s.z;
    r.m[1] = u.x;  r.m[5] = u.y;  r.m[9]  = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -v3_punto(s, ojo);
    r.m[13] = -v3_punto(u, ojo);
    r.m[14] =  v3_punto(f, ojo);
    return r;
}

Mat4 mat4_perspectiva(float fov_grados, float aspecto, float cerca, float lejos)
{
    float f = 1.0f / tanf(fov_grados * GRADOS_A_RAD * 0.5f);
    Mat4 r = {{ 0 }};
    r.m[0]  = f / aspecto;
    r.m[5]  = f;
    r.m[10] = lejos / (cerca - lejos);
    // Este -1 es la clave: copia -z a la componente w. La GPU despues divide
    // por w, o sea por la profundidad, y ahi es donde lo lejano se achica.
    r.m[11] = -1.0f;
    r.m[14] = (lejos * cerca) / (cerca - lejos);
    return r;
}
