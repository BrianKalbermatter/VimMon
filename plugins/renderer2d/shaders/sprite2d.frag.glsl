#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;
layout(location = 0) out vec4 color;

layout(set = 2, binding = 0) uniform sampler2D textura;

void main()
{
    vec4 c = texture(textura, v_uv) * v_color;
    // Totalmente transparente no escribe nada. El resto se MEZCLA con lo de
    // abajo (blend en el pipeline): la sombra al pie de un muro es un
    // degradado de alfa, y con solo discard saldria cortada a la mitad.
    if (c.a <= 0.0) discard;
    color = c;
}
