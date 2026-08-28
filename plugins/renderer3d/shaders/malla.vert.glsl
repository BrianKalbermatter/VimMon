#version 450
// Geometria del mundo: cubos y planos con profundidad.
//
// BINDINGS: SDL_GPU exige sets fijos para SPIR-V.
//   vertex   -> uniform buffers en set = 1
//   fragment -> texturas en set = 2, uniforms en set = 3
// Poner otro numero no da error: simplemente no se dibuja nada.

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out float out_luz;

layout(set = 1, binding = 0) uniform Camara {
    mat4 mvp;      // modelo * vista * proyeccion, ya multiplicadas en la CPU
    mat4 modelo;   // solo el modelo: para rotar la normal
    vec4 luz_dir;  // xyz = direccion de la luz, w sin usar
} u;

// La normal viaja como atributo del vertice. Se rota con el MODELO, no con
// la mvp: una normal dice hacia donde MIRA una cara, y eso no cambia segun
// donde este parada la camara.
//
// Ojo: mat3(modelo) lleva la escala adentro. Para escalas no uniformes lo
// correcto es la inversa traspuesta. Con cubos y planos alineados a los ejes
// -que es TODO lo que dibuja este motor- normalizar despues alcanza.
void main() {
    out_uv = in_uv;

    vec3 normal = normalize(mat3(u.modelo) * in_normal);

    // Luz plana en 3 escalones, no un degrade suave. En pixel art un
    // degrade continuo se ve sucio: se quiere que cada cara tenga SU tono.
    float d = max(dot(normal, normalize(-u.luz_dir.xyz)), 0.0);
    out_luz = d > 0.66 ? 1.0 : (d > 0.33 ? 0.75 : 0.5);

    gl_Position = u.mvp * vec4(in_pos, 1.0);
}
