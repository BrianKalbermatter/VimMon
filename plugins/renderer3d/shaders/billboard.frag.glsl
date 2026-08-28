#version 450
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

layout(set = 2, binding = 0) uniform sampler2D tex;
layout(set = 3, binding = 0) uniform Tinte { vec4 tinte; } t;

// RECORTE POR ALFA (discard), NO MEZCLA. Y esto es una decision, no un atajo.
//
// Mezclar (alpha blending) obliga a dibujar los sprites de atras hacia
// adelante, ordenados por distancia, cada frame. Ordenar 200 enemigos por
// frame cuesta, y si te equivocas se ven halos y recortes raros.
//
// Con discard el pixel transparente simplemente NO EXISTE: no escribe color
// ni profundidad. El z-buffer resuelve el orden solo, gratis, y se puede
// dibujar en cualquier orden.
//
// Se puede hacer porque el pixel art no tiene transparencias suaves: un
// pixel esta, o no esta. La limitacion del estilo ES la que habilita la
// solucion simple.
void main() {
    vec4 c = texture(tex, uv) * t.tinte;
    if (c.a < 0.5) discard;
    color = c;
}
