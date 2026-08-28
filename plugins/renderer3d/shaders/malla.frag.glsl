#version 450
layout(location = 0) in vec2 uv;
layout(location = 1) in float luz;

layout(location = 0) out vec4 color;

layout(set = 2, binding = 0) uniform sampler2D tex;
layout(set = 3, binding = 0) uniform Tinte { vec4 tinte; } t;

void main() {
    vec4 c = texture(tex, uv) * t.tinte;
    // Recorte por alfa, no mezcla. Ver el comentario largo en billboard.frag.
    if (c.a < 0.5) discard;
    color = vec4(c.rgb * luz, 1.0);
}
