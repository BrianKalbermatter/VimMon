#version 450
// Cuadrilateros 2D: la posicion llega en PIXELES del lienzo y aca se pasa
// a coordenadas de recorte (-1..1). La camara ya la resto la CPU al anotar
// cada vertice, asi que el shader no sabe nada del mundo.

layout(location = 0) in vec2 in_pos;    // px del lienzo, (0,0) arriba a la izquierda
layout(location = 1) in vec2 in_uv;     // 0..1 dentro de la textura
layout(location = 2) in vec4 in_color;  // tinte, ya normalizado a 0..1

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_color;

// SDL_GPU con SPIR-V: los uniforms del vertex shader van en el set 1.
layout(set = 1, binding = 0) uniform ULienzo {
    vec4 lienzo;   // xy = ancho y alto del lienzo en px
};

void main()
{
    // En las coordenadas de recorte de SDL_GPU +Y apunta para ARRIBA, y en
    // el lienzo +Y apunta para ABAJO (como en cualquier imagen). Por eso
    // la Y se da vuelta.
    vec2 ndc = vec2(in_pos.x / lienzo.x * 2.0 - 1.0,
                    1.0 - in_pos.y / lienzo.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    v_uv = in_uv;
    v_color = in_color;
}
