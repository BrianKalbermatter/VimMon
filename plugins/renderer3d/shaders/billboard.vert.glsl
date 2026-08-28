#version 450
// EL TRUCO DEL BILLBOARD, entero, en cuatro lineas.
//
// La idea ingenua es "rotar el sprite para que apunte a la camara": hay
// que calcular un angulo, y si la camara se mueve el angulo queda viejo.
//
// La idea buena es al reves: NO se rota nada. Se ARMA el cuadrado usando
// los ejes de la camara como reglas. Si los lados del cuadrado son el
// "derecha" y el "arriba" de la camara, entonces el cuadrado es paralelo
// a la pantalla por construccion. No puede NO mirarte.
//
// La CPU manda los dos ejes ya calculados. Mandando el "derecha" aplanado
// al piso y el "arriba" del mundo sale el billboard CILINDRICO: el sprite
// gira siguiendote pero nunca se acuesta, ni aunque mires al techo.
// Asi se paran los enemigos de DOOM.

layout(location = 0) in vec3 in_pos;   // cuadrado local: x,y en [-0.5, 0.5]
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec2 out_uv;

layout(set = 1, binding = 0) uniform Billboard {
    mat4 vista_proy;
    vec4 centro;    // xyz = posicion en el mundo,  w = ancho
    vec4 derecha;   // xyz = eje derecha de camara, w = alto
    vec4 arriba;    // xyz = eje arriba
} b;

void main() {
    out_uv = in_uv;
    vec3 p = b.centro.xyz
           + b.derecha.xyz * (in_pos.x * b.centro.w)
           + b.arriba.xyz  * (in_pos.y * b.derecha.w);
    gl_Position = b.vista_proy * vec4(p, 1.0);
}
