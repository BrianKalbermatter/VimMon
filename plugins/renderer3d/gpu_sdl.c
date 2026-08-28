// ============================================================
// VimMon — BACKEND 3D SOBRE SDL_GPU
//
// Implementa renderer3d.h. Todo el estado es static: afuera solo existe
// el vtable del final del archivo, igual que en sdl_fb.c.
//
// ── COMO DIBUJA UN FRAME ─────────────────────────────────────
//   1. se pide un command buffer (una LISTA de ordenes para la GPU)
//   2. se abre un render pass contra el lienzo CHICO (ej. 480x270)
//   3. se dibujan mallas y billboards ahi adentro
//   4. se cierra el pass y se BLITEA el lienzo chico estirado a la
//      ventana grande, con filtro NEAREST -> pixeles cuadrados
//   5. se envia el command buffer entero de una
//
// Con SDL_Render vos decias "dibuja esto" y pasaba. Aca vos ARMAS una
// lista de ordenes y despues la mandas. Es mas verboso, y a cambio la GPU
// recibe todo junto en vez de que la CPU la interrumpa mil veces.
// ============================================================

#include "renderer3d.h"
#include "math3d.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#ifndef RUTA_SHADERS
#define RUTA_SHADERS "plugins/renderer3d/shaders"
#endif

#define MAX_MALLAS    32
#define MAX_TEXTURAS  128

// Un vertice: posicion, coordenada de textura y NORMAL.
//
// La normal se manda explicita. La primera version intentaba deducirla de
// la posicion (normalize(in_pos)) para ahorrar 12 bytes por vertice, y
// estaba mal en los dos casos que dibuja el motor:
//   - en un plano XZ todos los vertices tienen y=0, asi que la componente
//     Y nunca puede ganar: el piso quedaba con la normal horizontal
//   - en un cubo las tres componentes valen lo mismo (0.577), asi que la
//     comparacion "cual es mayor" siempre elegia la misma
// Resultado: todo se dibujaba con el mismo tono. Ser vivo salio caro.
typedef struct { float x, y, z, u, v, nx, ny, nz; } Vertice;

typedef struct {
    SDL_GPUBuffer *vbuf, *ibuf;
    Uint32         n_indices;
} MallaGPU;

// ── Uniforms. El layout tiene que coincidir EXACTO con el del GLSL ──
// modelo va aparte de mvp porque la normal hay que rotarla con el modelo,
// no con la proyeccion: una normal indica hacia donde MIRA una cara, y eso
// no depende de donde este la camara.
typedef struct { Mat4 mvp; Mat4 modelo; float luz[4]; } UMallaVert;  // 144 bytes
typedef struct { float tinte[4];               } UTinteFrag;    // 16 bytes
typedef struct {
    Mat4  vista_proy;
    float centro[4];    // xyz pos, w ancho
    float derecha[4];   // xyz eje, w alto
    float arriba[4];
} UBillboardVert;                                               // 112 bytes

// ── Estado interno ──────────────────────────────────────────
static SDL_Window            *g_win  = NULL;
static SDL_GPUDevice         *g_dev  = NULL;
static SDL_GPUGraphicsPipeline *g_pipe_malla     = NULL;
static SDL_GPUGraphicsPipeline *g_pipe_billboard = NULL;
static SDL_GPUTexture        *g_lienzo = NULL;   // color, resolucion interna
static SDL_GPUTexture        *g_prof   = NULL;   // z-buffer
static SDL_GPUSampler        *g_sampler = NULL;
static SDL_GPUTextureFormat   g_fmt_prof = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

static MallaGPU      g_mallas[MAX_MALLAS];
static Uint32        g_n_mallas = 0;
static SDL_GPUTexture *g_texturas[MAX_TEXTURAS];
static Uint32        g_n_texturas = 0;

static Malla   g_cubo = MALLA_NULA, g_plano = MALLA_NULA, g_quad = MALLA_NULA;
static int     g_ancho_int = 0, g_alto_int = 0;
static int     g_ancho_win = 0, g_alto_win = 0;

// Estado del frame en curso
static SDL_GPUCommandBuffer *g_cmd  = NULL;
static SDL_GPURenderPass    *g_pass = NULL;
static Mat4 g_vista_proy;
static V3   g_cam_derecha, g_cam_arriba;

static const SDL_Scancode g_scan[T3_COUNT] = {
    [T3_ESCAPE] = SDL_SCANCODE_ESCAPE, [T3_SPACE] = SDL_SCANCODE_SPACE,
    [T3_UP] = SDL_SCANCODE_UP,   [T3_DOWN]  = SDL_SCANCODE_DOWN,
    [T3_LEFT] = SDL_SCANCODE_LEFT, [T3_RIGHT] = SDL_SCANCODE_RIGHT,
    [T3_W] = SDL_SCANCODE_W, [T3_A] = SDL_SCANCODE_A,
    [T3_S] = SDL_SCANCODE_S, [T3_D] = SDL_SCANCODE_D,
};

// ── Utilidades ──────────────────────────────────────────────
static void argb_a_float(uint32_t c, float out[4])
{
    out[0] = (float)((c >> 16) & 0xFF) / 255.0f;
    out[1] = (float)((c >>  8) & 0xFF) / 255.0f;
    out[2] = (float)( c        & 0xFF) / 255.0f;
    out[3] = (float)((c >> 24) & 0xFF) / 255.0f;
}

static SDL_GPUShader *
cargar_shader(const char *nombre, SDL_GPUShaderStage etapa,
              Uint32 n_samplers, Uint32 n_uniforms)
{
    char ruta[512];
    snprintf(ruta, sizeof(ruta), "%s/%s", RUTA_SHADERS, nombre);

    size_t largo = 0;
    void *codigo = SDL_LoadFile(ruta, &largo);
    if (!codigo) {
        fprintf(stderr, "[gpu] no pude leer %s: %s\n", ruta, SDL_GetError());
        return NULL;
    }

    SDL_GPUShaderCreateInfo info = {
        .code_size           = largo,
        .code                = (const Uint8 *)codigo,
        .entrypoint          = "main",
        .format              = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage               = etapa,
        .num_samplers        = n_samplers,
        .num_uniform_buffers = n_uniforms,
    };
    SDL_GPUShader *sh = SDL_CreateGPUShader(g_dev, &info);
    SDL_free(codigo);
    if (!sh) fprintf(stderr, "[gpu] CreateGPUShader %s: %s\n", nombre, SDL_GetError());
    return sh;
}

// Sube vertices e indices a la GPU. La memoria de la GPU no se escribe
// directo: se copia primero a un TRANSFER BUFFER (memoria que ven las dos)
// y de ahi la GPU se lo lleva adentro. Ese rodeo es el precio de que la
// memoria rapida de la GPU no sea accesible desde la CPU.
static Malla
subir_malla(const Vertice *vs, Uint32 n_v, const Uint16 *is, Uint32 n_i)
{
    if (g_n_mallas >= MAX_MALLAS) return MALLA_NULA;

    Uint32 bytes_v = n_v * (Uint32)sizeof(Vertice);
    Uint32 bytes_i = n_i * (Uint32)sizeof(Uint16);

    SDL_GPUBufferCreateInfo bv = { .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = bytes_v };
    SDL_GPUBufferCreateInfo bi = { .usage = SDL_GPU_BUFFERUSAGE_INDEX,  .size = bytes_i };
    SDL_GPUBuffer *vbuf = SDL_CreateGPUBuffer(g_dev, &bv);
    SDL_GPUBuffer *ibuf = SDL_CreateGPUBuffer(g_dev, &bi);
    if (!vbuf || !ibuf) return MALLA_NULA;

    SDL_GPUTransferBufferCreateInfo tinfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = bytes_v + bytes_i
    };
    SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(g_dev, &tinfo);
    if (!tb) return MALLA_NULA;

    Uint8 *mapa = SDL_MapGPUTransferBuffer(g_dev, tb, false);
    memcpy(mapa,           vs, bytes_v);
    memcpy(mapa + bytes_v, is, bytes_i);
    SDL_UnmapGPUTransferBuffer(g_dev, tb);

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(g_dev);
    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation src = { .transfer_buffer = tb, .offset = 0 };
    SDL_GPUBufferRegion dst = { .buffer = vbuf, .offset = 0, .size = bytes_v };
    SDL_UploadToGPUBuffer(cp, &src, &dst, false);
    src.offset = bytes_v;
    dst.buffer = ibuf; dst.size = bytes_i;
    SDL_UploadToGPUBuffer(cp, &src, &dst, false);
    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(g_dev, tb);

    g_mallas[g_n_mallas] = (MallaGPU){ vbuf, ibuf, n_i };
    g_n_mallas++;
    return (Malla)g_n_mallas;   // handle = indice + 1, para que 0 sea "nula"
}

static Textura
subir_textura(const uint32_t *pix, int w, int h)
{
    if (g_n_texturas >= MAX_TEXTURAS) return TEXTURA_NULA;

    SDL_GPUTextureCreateInfo ti = {
        .type                 = SDL_GPU_TEXTURETYPE_2D,
        .format               = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM, // = ARGB en LE
        .usage                = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width                = (Uint32)w,
        .height               = (Uint32)h,
        .layer_count_or_depth = 1,
        .num_levels           = 1,
    };
    SDL_GPUTexture *tex = SDL_CreateGPUTexture(g_dev, &ti);
    if (!tex) return TEXTURA_NULA;

    Uint32 bytes = (Uint32)(w * h * 4);
    SDL_GPUTransferBufferCreateInfo tbi = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = bytes
    };
    SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(g_dev, &tbi);
    void *mapa = SDL_MapGPUTransferBuffer(g_dev, tb, false);
    memcpy(mapa, pix, bytes);
    SDL_UnmapGPUTransferBuffer(g_dev, tb);

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(g_dev);
    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureTransferInfo src = {
        .transfer_buffer = tb, .offset = 0,
        .pixels_per_row = (Uint32)w, .rows_per_layer = (Uint32)h
    };
    SDL_GPUTextureRegion dst = {
        .texture = tex, .w = (Uint32)w, .h = (Uint32)h, .d = 1
    };
    SDL_UploadToGPUTexture(cp, &src, &dst, false);
    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(g_dev, tb);

    g_texturas[g_n_texturas] = tex;
    g_n_texturas++;
    return (Textura)g_n_texturas;
}

// ── Geometria base ──────────────────────────────────────────
static Malla crear_cubo(void)
{
    // 24 vertices y no 8: cada cara necesita SUS coordenadas de textura,
    // y un vertice compartido entre tres caras no puede tener tres UV.
    // pos                 uv     normal
    static const Vertice v[24] = {
        {-.5f,-.5f, .5f, 0,1,  0, 0, 1},{ .5f,-.5f, .5f, 1,1,  0, 0, 1},
        { .5f, .5f, .5f, 1,0,  0, 0, 1},{-.5f, .5f, .5f, 0,0,  0, 0, 1},   // +Z
        { .5f,-.5f,-.5f, 0,1,  0, 0,-1},{-.5f,-.5f,-.5f, 1,1,  0, 0,-1},
        {-.5f, .5f,-.5f, 1,0,  0, 0,-1},{ .5f, .5f,-.5f, 0,0,  0, 0,-1},   // -Z
        { .5f,-.5f, .5f, 0,1,  1, 0, 0},{ .5f,-.5f,-.5f, 1,1,  1, 0, 0},
        { .5f, .5f,-.5f, 1,0,  1, 0, 0},{ .5f, .5f, .5f, 0,0,  1, 0, 0},   // +X
        {-.5f,-.5f,-.5f, 0,1, -1, 0, 0},{-.5f,-.5f, .5f, 1,1, -1, 0, 0},
        {-.5f, .5f, .5f, 1,0, -1, 0, 0},{-.5f, .5f,-.5f, 0,0, -1, 0, 0},   // -X
        {-.5f, .5f, .5f, 0,1,  0, 1, 0},{ .5f, .5f, .5f, 1,1,  0, 1, 0},
        { .5f, .5f,-.5f, 1,0,  0, 1, 0},{-.5f, .5f,-.5f, 0,0,  0, 1, 0},   // +Y
        {-.5f,-.5f,-.5f, 0,1,  0,-1, 0},{ .5f,-.5f,-.5f, 1,1,  0,-1, 0},
        { .5f,-.5f, .5f, 1,0,  0,-1, 0},{-.5f,-.5f, .5f, 0,0,  0,-1, 0},   // -Y
    };
    static Uint16 i[36];
    for (Uint16 c = 0; c < 6; c++) {
        Uint16 b = (Uint16)(c * 4);
        i[c*6+0]=b; i[c*6+1]=(Uint16)(b+1); i[c*6+2]=(Uint16)(b+2);
        i[c*6+3]=b; i[c*6+4]=(Uint16)(b+2); i[c*6+5]=(Uint16)(b+3);
    }
    return subir_malla(v, 24, i, 36);
}

static Malla crear_quad_xy(void)
{
    static const Vertice v[4] = {
        {-.5f,-.5f,0, 0,1, 0,0,1},{ .5f,-.5f,0, 1,1, 0,0,1},
        { .5f, .5f,0, 1,0, 0,0,1},{-.5f, .5f,0, 0,0, 0,0,1}
    };
    static const Uint16 i[6] = { 0,1,2, 0,2,3 };
    return subir_malla(v, 4, i, 6);
}

static Malla crear_plano_xz(void)
{
    // Normal (0,1,0): un plano XZ mira para arriba. Explicito, no deducido.
    static const Vertice v[4] = {
        {-.5f,0,-.5f, 0,0, 0,1,0},{ .5f,0,-.5f, 1,0, 0,1,0},
        { .5f,0, .5f, 1,1, 0,1,0},{-.5f,0, .5f, 0,1, 0,1,0}
    };
    static const Uint16 i[6] = { 0,1,2, 0,2,3 };
    return subir_malla(v, 4, i, 6);
}

// ── Pipelines ───────────────────────────────────────────────
static SDL_GPUGraphicsPipeline *
crear_pipeline(const char *vert, const char *frag, Uint32 uniforms_vert)
{
    SDL_GPUShader *vs = cargar_shader(vert, SDL_GPU_SHADERSTAGE_VERTEX, 0, uniforms_vert);
    SDL_GPUShader *fs = cargar_shader(frag, SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
    if (!vs || !fs) return NULL;

    SDL_GPUVertexBufferDescription vbd = {
        .slot = 0, .pitch = sizeof(Vertice),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    };
    SDL_GPUVertexAttribute attrs[3] = {
        { .location = 0, .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0 },
        { .location = 1, .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = sizeof(float) * 3 },
        // El shader de billboard no usa la normal. Declarar un atributo que
        // un shader ignora es legal: solo se desperdicia el ancho de banda.
        { .location = 2, .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = sizeof(float) * 5 },
    };
    SDL_GPUColorTargetDescription ctd = {
        .format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
    };

    SDL_GPUGraphicsPipelineCreateInfo pi = {
        .vertex_shader   = vs,
        .fragment_shader = fs,
        .vertex_input_state = {
            .vertex_buffer_descriptions = &vbd, .num_vertex_buffers = 1,
            .vertex_attributes = attrs,         .num_vertex_attributes = 3,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            // Sin descarte de caras traseras: los billboards son planos y
            // se verian desde un solo lado. Un motor grande separaria esto
            // en dos pipelines; aca no vale la pena.
            .cull_mode = SDL_GPU_CULLMODE_NONE,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        },
        .depth_stencil_state = {
            // LESS: solo se pinta si esta MAS CERCA que lo ya pintado.
            // Ese es el z-buffer, y es lo que hace que no haya que ordenar
            // nada a mano como en el scene_view.c del backend 2D.
            .compare_op         = SDL_GPU_COMPAREOP_LESS,
            .enable_depth_test  = true,
            .enable_depth_write = true,
        },
        .target_info = {
            .color_target_descriptions = &ctd,
            .num_color_targets         = 1,
            .depth_stencil_format      = g_fmt_prof,
            .has_depth_stencil_target  = true,
        },
    };

    SDL_GPUGraphicsPipeline *p = SDL_CreateGPUGraphicsPipeline(g_dev, &pi);
    if (!p) fprintf(stderr, "[gpu] CreateGraphicsPipeline: %s\n", SDL_GetError());
    // Los shaders ya viven adentro del pipeline: se pueden soltar.
    SDL_ReleaseGPUShader(g_dev, vs);
    SDL_ReleaseGPUShader(g_dev, fs);
    return p;
}

// ── Ciclo de vida ───────────────────────────────────────────
static int gpu_init(const char *titulo, int aw, int ah, int ai, int hi)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "[gpu] SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    g_dev = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
    if (!g_dev) {
        fprintf(stderr, "[gpu] CreateGPUDevice: %s\n", SDL_GetError());
        fprintf(stderr, "[gpu] falta un driver Vulkan (ICD). Ver el README.\n");
        return 1;
    }
    g_win = SDL_CreateWindow(titulo, aw, ah, 0);
    if (!g_win || !SDL_ClaimWindowForGPUDevice(g_dev, g_win)) {
        fprintf(stderr, "[gpu] ventana/swapchain: %s\n", SDL_GetError());
        return 1;
    }
    g_ancho_win = aw; g_alto_win = ah;
    g_ancho_int = ai; g_alto_int = hi;

    // No todos los drivers soportan el mismo formato de profundidad:
    // se pregunta en vez de asumir.
    const SDL_GPUTextureFormat candidatos[] = {
        SDL_GPU_TEXTUREFORMAT_D24_UNORM,
        SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        SDL_GPU_TEXTUREFORMAT_D16_UNORM,
    };
    for (size_t i = 0; i < SDL_arraysize(candidatos); i++) {
        if (SDL_GPUTextureSupportsFormat(g_dev, candidatos[i],
                SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
            g_fmt_prof = candidatos[i];
            break;
        }
    }

    SDL_GPUTextureCreateInfo lienzo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = (Uint32)ai, .height = (Uint32)hi,
        .layer_count_or_depth = 1, .num_levels = 1,
    };
    g_lienzo = SDL_CreateGPUTexture(g_dev, &lienzo);

    SDL_GPUTextureCreateInfo prof = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = g_fmt_prof,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = (Uint32)ai, .height = (Uint32)hi,
        .layer_count_or_depth = 1, .num_levels = 1,
    };
    g_prof = SDL_CreateGPUTexture(g_dev, &prof);
    if (!g_lienzo || !g_prof) {
        fprintf(stderr, "[gpu] no pude crear el lienzo interno: %s\n", SDL_GetError());
        return 1;
    }

    // NEAREST en los dos filtros y CLAMP en los bordes. Esto es lo que
    // hace que una textura de 16x16 se vea como 16 cuadrados nitidos y
    // no como una mancha suave.
    SDL_GPUSamplerCreateInfo si = {
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };
    g_sampler = SDL_CreateGPUSampler(g_dev, &si);

    g_pipe_malla     = crear_pipeline("malla.vert.spv",     "malla.frag.spv",     1);
    g_pipe_billboard = crear_pipeline("billboard.vert.spv", "billboard.frag.spv", 1);
    if (!g_pipe_malla || !g_pipe_billboard) return 1;

    g_cubo  = crear_cubo();
    g_plano = crear_plano_xz();
    g_quad  = crear_quad_xy();
    return 0;
}

static void gpu_shutdown(void)
{
    if (!g_dev) return;
    for (Uint32 i = 0; i < g_n_mallas; i++) {
        SDL_ReleaseGPUBuffer(g_dev, g_mallas[i].vbuf);
        SDL_ReleaseGPUBuffer(g_dev, g_mallas[i].ibuf);
    }
    for (Uint32 i = 0; i < g_n_texturas; i++)
        SDL_ReleaseGPUTexture(g_dev, g_texturas[i]);
    if (g_sampler)         SDL_ReleaseGPUSampler(g_dev, g_sampler);
    if (g_lienzo)          SDL_ReleaseGPUTexture(g_dev, g_lienzo);
    if (g_prof)            SDL_ReleaseGPUTexture(g_dev, g_prof);
    if (g_pipe_malla)      SDL_ReleaseGPUGraphicsPipeline(g_dev, g_pipe_malla);
    if (g_pipe_billboard)  SDL_ReleaseGPUGraphicsPipeline(g_dev, g_pipe_billboard);
    if (g_win)             SDL_ReleaseWindowFromGPUDevice(g_dev, g_win);
    SDL_DestroyGPUDevice(g_dev);
    if (g_win) SDL_DestroyWindow(g_win);
    g_dev = NULL; g_win = NULL;
    SDL_Quit();
}

// ── Recursos publicos ───────────────────────────────────────
static Malla   gpu_malla_cubo(void)  { return g_cubo;  }
static Malla   gpu_malla_plano(void) { return g_plano; }

static Textura gpu_textura_solida(uint32_t argb)
{
    return subir_textura(&argb, 1, 1);
}
static Textura gpu_textura_desde_pixeles(const uint32_t *p, int w, int h)
{
    if (!p || w <= 0 || h <= 0) return TEXTURA_NULA;
    return subir_textura(p, w, h);
}

// ── Un frame ────────────────────────────────────────────────
static void gpu_frame_begin(const Camara3D *cam, uint32_t cielo)
{
    g_cmd = SDL_AcquireGPUCommandBuffer(g_dev);
    if (!g_cmd) return;

    float aspecto = (float)g_ancho_int / (float)g_alto_int;
    Mat4 vista = mat4_mirar(cam->pos, cam->mira_a, v3(0, 1, 0));
    Mat4 proy  = mat4_perspectiva(cam->fov_grados, aspecto, cam->cerca, cam->lejos);
    g_vista_proy = mat4_mul(proy, vista);

    // Ejes para los billboards, estilo CILINDRICO: el "derecha" de la
    // camara aplanado contra el piso, y el "arriba" del mundo. Asi el
    // sprite te sigue girando pero nunca se acuesta.
    V3 ade = v3_normal(v3_sub(cam->mira_a, cam->pos));
    g_cam_derecha = v3_normal(v3_cruz(ade, v3(0, 1, 0)));
    g_cam_arriba  = v3(0, 1, 0);

    float c[4]; argb_a_float(cielo, c);
    SDL_GPUColorTargetInfo ct = {
        .texture = g_lienzo,
        .clear_color = (SDL_FColor){ c[0], c[1], c[2], c[3] },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPUDepthStencilTargetInfo dt = {
        .texture = g_prof,
        .clear_depth = 1.0f,     // 1.0 = infinitamente lejos
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_DONT_CARE,
        .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
        .cycle = true,
    };
    g_pass = SDL_BeginGPURenderPass(g_cmd, &ct, 1, &dt);
}

static void
bind_malla_y_textura(SDL_GPUGraphicsPipeline *pipe, Malla m, Textura t)
{
    SDL_BindGPUGraphicsPipeline(g_pass, pipe);

    MallaGPU *mg = &g_mallas[m - 1];
    SDL_GPUBufferBinding vb = { .buffer = mg->vbuf, .offset = 0 };
    SDL_GPUBufferBinding ib = { .buffer = mg->ibuf, .offset = 0 };
    SDL_BindGPUVertexBuffers(g_pass, 0, &vb, 1);
    SDL_BindGPUIndexBuffer(g_pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_GPUTextureSamplerBinding ts = {
        .texture = g_texturas[t - 1], .sampler = g_sampler
    };
    SDL_BindGPUFragmentSamplers(g_pass, 0, &ts, 1);
}

static void
gpu_dibujar_malla(Malla m, V3 pos, V3 rot, V3 esc, Textura t, uint32_t tinte)
{
    // Guard clauses: un handle invalido no puede llegar a indexar el array.
    if (!g_pass || m == MALLA_NULA || m > g_n_mallas) return;
    if (t == TEXTURA_NULA || t > g_n_texturas) return;

    bind_malla_y_textura(g_pipe_malla, m, t);

    UMallaVert uv;
    Mat4 modelo = mat4_modelo(pos, rot, esc);
    uv.modelo = modelo;
    uv.mvp = mat4_mul(g_vista_proy, modelo);
    uv.luz[0] = -0.4f; uv.luz[1] = -1.0f; uv.luz[2] = -0.3f; uv.luz[3] = 0.0f;
    SDL_PushGPUVertexUniformData(g_cmd, 0, &uv, sizeof(uv));

    UTinteFrag uf; argb_a_float(tinte, uf.tinte);
    SDL_PushGPUFragmentUniformData(g_cmd, 0, &uf, sizeof(uf));

    SDL_DrawGPUIndexedPrimitives(g_pass, g_mallas[m - 1].n_indices, 1, 0, 0, 0);
}

static void
gpu_dibujar_billboard(Textura t, V3 pos, float ancho, float alto, uint32_t tinte)
{
    if (!g_pass || t == TEXTURA_NULA || t > g_n_texturas) return;
    if (g_quad == MALLA_NULA) return;

    bind_malla_y_textura(g_pipe_billboard, g_quad, t);

    UBillboardVert ub;
    ub.vista_proy = g_vista_proy;
    ub.centro[0]  = pos.x; ub.centro[1] = pos.y; ub.centro[2] = pos.z;
    ub.centro[3]  = ancho;
    ub.derecha[0] = g_cam_derecha.x; ub.derecha[1] = g_cam_derecha.y;
    ub.derecha[2] = g_cam_derecha.z; ub.derecha[3] = alto;
    ub.arriba[0]  = g_cam_arriba.x;  ub.arriba[1] = g_cam_arriba.y;
    ub.arriba[2]  = g_cam_arriba.z;  ub.arriba[3] = 0.0f;
    SDL_PushGPUVertexUniformData(g_cmd, 0, &ub, sizeof(ub));

    UTinteFrag uf; argb_a_float(tinte, uf.tinte);
    SDL_PushGPUFragmentUniformData(g_cmd, 0, &uf, sizeof(uf));

    SDL_DrawGPUIndexedPrimitives(g_pass, g_mallas[g_quad - 1].n_indices, 1, 0, 0, 0);
}

static void gpu_frame_end(void)
{
    if (!g_cmd) return;
    if (g_pass) { SDL_EndGPURenderPass(g_pass); g_pass = NULL; }

    SDL_GPUTexture *sw = NULL;
    Uint32 sw_w = 0, sw_h = 0;
    if (SDL_WaitAndAcquireGPUSwapchainTexture(g_cmd, g_win, &sw, &sw_w, &sw_h) && sw) {
        // ACA esta el pixel art: el lienzo chico se estira a la ventana
        // entera con NEAREST. Cada pixel interno se convierte en un
        // cuadrado grande de color plano, sin interpolar con el vecino.
        SDL_GPUBlitInfo bi = {
            .source      = { .texture = g_lienzo, .w = (Uint32)g_ancho_int,
                             .h = (Uint32)g_alto_int },
            .destination = { .texture = sw, .w = sw_w, .h = sw_h },
            .load_op     = SDL_GPU_LOADOP_DONT_CARE,
            .filter      = SDL_GPU_FILTER_NEAREST,
        };
        SDL_BlitGPUTexture(g_cmd, &bi);
    }
    SDL_SubmitGPUCommandBuffer(g_cmd);
    g_cmd = NULL;
}

// ── Plataforma ──────────────────────────────────────────────

// Movimiento del mouse ACUMULADO desde la ultima lectura.
//
// Tiene que acumularse aca y no leerse cuando lo piden, y la razon es la cola
// de eventos de SDL: en un frame el sistema puede meter VARIOS eventos de
// movimiento, y poll_quit los saca todos de la cola de una. El que vacia la
// cola es el unico que llega a verlos: si no los sumara, se perderian y el
// mouse se sentiria trabado. Por eso el juego DEBE llamar poll_quit una vez
// por frame, aunque no le importe si se cerro la ventana.
static float g_mdx = 0.0f, g_mdy = 0.0f;

static int gpu_poll_quit(void)
{
    SDL_Event e; int salir = 0;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) salir = 1;
        else if (e.type == SDL_EVENT_MOUSE_MOTION) {
            g_mdx += e.motion.xrel;
            g_mdy += e.motion.yrel;
        }
    }
    return salir;
}
static int gpu_tecla(Tecla3D k)
{
    if (k < 0 || k >= T3_COUNT) return 0;
    const bool *st = SDL_GetKeyboardState(NULL);
    return st[g_scan[k]] ? 1 : 0;
}

// Devuelve lo acumulado y lo pone en cero: lo leido ya se uso, y dejarlo
// sumaria dos veces el mismo movimiento.
static void gpu_mouse_delta(float *dx, float *dy)
{
    if (dx) *dx = g_mdx;
    if (dy) *dy = g_mdy;
    g_mdx = 0.0f;
    g_mdy = 0.0f;
}

static int gpu_mouse_boton(Boton3D b)
{
    if (b < 0 || b >= M3_COUNT) return 0;
    static const int sdl_btn[M3_COUNT] = {
        SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT, SDL_BUTTON_MIDDLE
    };
    SDL_MouseButtonFlags st = SDL_GetMouseState(NULL, NULL);
    return (st & SDL_BUTTON_MASK(sdl_btn[b])) ? 1 : 0;
}

static void gpu_mouse_capturar(int activar)
{
    if (!g_win) return;
    SDL_SetWindowRelativeMouseMode(g_win, activar ? true : false);
    // Al entrar o salir del modo relativo SDL puede emitir un salto grande.
    // Descartarlo evita que la camara pegue un tiron el primer frame.
    g_mdx = 0.0f;
    g_mdy = 0.0f;
}
static uint32_t gpu_ticks(void)          { return (uint32_t)SDL_GetTicks(); }
static void     gpu_delay(uint32_t ms)   { SDL_Delay(ms); }
static int      gpu_ancho_interno(void)  { return g_ancho_int; }
static int      gpu_alto_interno(void)   { return g_alto_int;  }


// ── Verificar sin pantalla ──────────────────────────────────
int gpu_sdl_capturar_bmp(const char *ruta)
{
    if (!g_dev || !g_lienzo) return 1;

    Uint32 bytes = (Uint32)(g_ancho_int * g_alto_int * 4);
    SDL_GPUTransferBufferCreateInfo ti = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, .size = bytes
    };
    SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(g_dev, &ti);
    if (!tb) return 1;

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(g_dev);
    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureRegion src = {
        .texture = g_lienzo, .w = (Uint32)g_ancho_int,
        .h = (Uint32)g_alto_int, .d = 1
    };
    SDL_GPUTextureTransferInfo dst = {
        .transfer_buffer = tb, .offset = 0,
        .pixels_per_row = (Uint32)g_ancho_int, .rows_per_layer = (Uint32)g_alto_int
    };
    SDL_DownloadFromGPUTexture(cp, &src, &dst);
    SDL_EndGPUCopyPass(cp);

    // La CPU y la GPU corren en paralelo: submit NO espera. Sin la fence
    // leeriamos el buffer antes de que la GPU lo haya llenado, y saldria
    // basura o negro. La fence es "avisame cuando REALMENTE terminaste".
    SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    SDL_WaitForGPUFences(g_dev, true, &fence, 1);
    SDL_ReleaseGPUFence(g_dev, fence);

    void *pix = SDL_MapGPUTransferBuffer(g_dev, tb, false);
    int ok = 1;
    if (pix) {
        SDL_Surface *sup = SDL_CreateSurfaceFrom(
            g_ancho_int, g_alto_int, SDL_PIXELFORMAT_ARGB8888,
            pix, g_ancho_int * 4);
        if (sup) {
            ok = SDL_SaveBMP(sup, ruta) ? 0 : 1;
            SDL_DestroySurface(sup);
        }
        SDL_UnmapGPUTransferBuffer(g_dev, tb);
    }
    SDL_ReleaseGPUTransferBuffer(g_dev, tb);
    if (ok) fprintf(stderr, "[gpu] captura fallo: %s\n", SDL_GetError());
    return ok;
}

// ── El vtable: lo unico publico ─────────────────────────────
Renderer3D gpu_sdl_renderer3d = {
    .init                   = gpu_init,
    .shutdown               = gpu_shutdown,
    .malla_cubo             = gpu_malla_cubo,
    .malla_plano            = gpu_malla_plano,
    .textura_solida         = gpu_textura_solida,
    .textura_desde_pixeles  = gpu_textura_desde_pixeles,
    .frame_begin            = gpu_frame_begin,
    .dibujar_malla          = gpu_dibujar_malla,
    .dibujar_billboard      = gpu_dibujar_billboard,
    .frame_end              = gpu_frame_end,
    .poll_quit              = gpu_poll_quit,
    .tecla                  = gpu_tecla,
    .mouse_delta            = gpu_mouse_delta,
    .mouse_boton            = gpu_mouse_boton,
    .mouse_capturar         = gpu_mouse_capturar,
    .ticks_ms               = gpu_ticks,
    .delay_ms               = gpu_delay,
    .ancho_interno          = gpu_ancho_interno,
    .alto_interno           = gpu_alto_interno,
};
