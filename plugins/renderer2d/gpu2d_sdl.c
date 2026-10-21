// ============================================================
// VimMon — BACKEND 2D SOBRE SDL_GPU
//
// Implementa renderer2d.h. Todo el estado es static: afuera solo existe
// el vtable del final, igual que en gpu_sdl.c del 3D.
//
// ── LA DIFERENCIA CON EL 3D: LOTES ───────────────────────────
// El 3D hace un draw por objeto: tiene pocos objetos. Un frame 2D de
// Delirio tiene MILES de cuadrilateros (cada tile del piso, cada cara de
// cada muro). Un draw por cada uno ahogaria a la CPU hablando con el
// driver. Entonces:
//   1. durante el frame cada cuadrilatero se ANOTA en un arreglo en RAM
//      (4 vertices). Cuando cambia la textura se abre un "lote" nuevo.
//   2. en frame_end se sube el arreglo entero a la GPU de una vez
//   3. se hace UN draw por lote. Con un atlas, casi todo es un lote.
//
// ── COMO TERMINA UN FRAME ────────────────────────────────────
//   copy pass   -> los vertices viajan de RAM a la GPU
//   render pass -> los lotes se pintan en el lienzo chico
//   blit        -> el lienzo se estira por un factor ENTERO a la ventana
//   submit      -> todo junto
// ============================================================

#include "renderer2d.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef RUTA_SHADERS_2D
#define RUTA_SHADERS_2D "plugins/renderer2d/shaders"
#endif

#define MAX_TEXTURAS 256
#define MAX_CUADS    65536          // por frame; 4 vertices y 6 indices cada uno
#define MAX_LOTES    4096

// Un vertice 2D: 20 bytes. El color va en 4 bytes (no 4 floats): con
// decenas de miles de vertices por frame, el ancho de banda importa.
typedef struct { float x, y, u, v; uint32_t rgba; } Vertice2D;

typedef struct { SDL_GPUTexture *tex; int w, h; } TexturaGPU;

// Un lote: cuadrilateros seguidos que comparten textura.
typedef struct { Textura2D tex; Uint32 primero, cantidad; } Lote;

typedef struct { float lienzo[4]; } ULienzo;    // 16 bytes, igual que el GLSL

// ── Estado interno ──────────────────────────────────────────
static SDL_Window              *g_win = NULL;
static SDL_GPUDevice           *g_dev = NULL;
static SDL_GPUGraphicsPipeline *g_pipe = NULL;
static SDL_GPUTexture          *g_lienzo = NULL;
static SDL_GPUSampler          *g_sampler = NULL;
static SDL_GPUBuffer           *g_vbuf = NULL, *g_ibuf = NULL;
static SDL_GPUTransferBuffer   *g_subida = NULL;

static TexturaGPU g_texturas[MAX_TEXTURAS];
static Uint32     g_n_texturas = 0;
static Textura2D  g_blanca = TEXTURA2D_NULA;   // 1x1 para los colores lisos

static Vertice2D *g_verts = NULL;               // MAX_CUADS * 4, en RAM
static Uint32     g_n_cuads = 0;
static Lote       g_lotes[MAX_LOTES];
static Uint32     g_n_lotes = 0;

static int      g_alto_objetivo = 480;
static int      g_ancho_int = 0, g_alto_int = 0, g_escala = 1;
static float    g_cam_x = 0, g_cam_y = 0;
static uint32_t g_fondo = 0xFF000000u;
static int      g_avisado_lleno = 0;

// ── Utilidades ──────────────────────────────────────────────

// ARGB (como lo piensa el juego) -> bytes R,G,B,A en memoria (como lo lee
// el atributo UBYTE4_NORM). En little endian el primer byte es el de abajo.
static uint32_t argb_a_rgba(uint32_t c)
{
    uint32_t a = (c >> 24) & 0xFF, r = (c >> 16) & 0xFF;
    uint32_t g = (c >> 8) & 0xFF,  b = c & 0xFF;
    return r | (g << 8) | (b << 16) | (a << 24);
}

static SDL_GPUShader *
cargar_shader(const char *nombre, SDL_GPUShaderStage etapa,
              Uint32 n_samplers, Uint32 n_uniforms)
{
    char ruta[512];
    snprintf(ruta, sizeof(ruta), "%s/%s", RUTA_SHADERS_2D, nombre);

    size_t largo = 0;
    void *codigo = SDL_LoadFile(ruta, &largo);
    if (!codigo) {
        fprintf(stderr, "[gpu2d] no pude leer %s: %s\n", ruta, SDL_GetError());
        return NULL;
    }
    SDL_GPUShaderCreateInfo info = {
        .code_size = largo, .code = (const Uint8 *)codigo, .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV, .stage = etapa,
        .num_samplers = n_samplers, .num_uniform_buffers = n_uniforms,
    };
    SDL_GPUShader *sh = SDL_CreateGPUShader(g_dev, &info);
    SDL_free(codigo);
    if (!sh) fprintf(stderr, "[gpu2d] CreateGPUShader %s: %s\n", nombre, SDL_GetError());
    return sh;
}

static Textura2D subir_textura(const uint32_t *pix, int w, int h)
{
    if (g_n_texturas >= MAX_TEXTURAS) return TEXTURA2D_NULA;

    SDL_GPUTextureCreateInfo ti = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,   // = ARGB en LE
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = (Uint32)w, .height = (Uint32)h,
        .layer_count_or_depth = 1, .num_levels = 1,
    };
    SDL_GPUTexture *tex = SDL_CreateGPUTexture(g_dev, &ti);
    if (!tex) return TEXTURA2D_NULA;

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
        .transfer_buffer = tb, .pixels_per_row = (Uint32)w, .rows_per_layer = (Uint32)h
    };
    SDL_GPUTextureRegion dst = { .texture = tex, .w = (Uint32)w, .h = (Uint32)h, .d = 1 };
    SDL_UploadToGPUTexture(cp, &src, &dst, false);
    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(g_dev, tb);

    g_texturas[g_n_texturas] = (TexturaGPU){ tex, w, h };
    g_n_texturas++;
    return (Textura2D)g_n_texturas;   // handle = indice + 1, 0 es "nula"
}

// El lienzo interno sale del tamano REAL de la ventana en pixeles (con
// pantallas HiDPI no es el mismo que el tamano logico). Se rehace cuando
// la ventana cambia de tamano.
static int rehacer_lienzo(void)
{
    int pw = 0, ph = 0;
    SDL_GetWindowSizeInPixels(g_win, &pw, &ph);
    if (pw <= 0 || ph <= 0) return 0;

    int escala = (int)lroundf((float)ph / (float)g_alto_objetivo);
    if (escala < 1) escala = 1;
    // Hacia abajo, a diferencia del navegador (que redondea hacia arriba y
    // deja que el borde se recorte): el blit no puede salirse de la ventana.
    int ai = pw / escala, hi = ph / escala;
    if (g_lienzo && ai == g_ancho_int && hi == g_alto_int && escala == g_escala) return 0;

    if (g_lienzo) SDL_ReleaseGPUTexture(g_dev, g_lienzo);
    SDL_GPUTextureCreateInfo li = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = (Uint32)ai, .height = (Uint32)hi,
        .layer_count_or_depth = 1, .num_levels = 1,
    };
    g_lienzo = SDL_CreateGPUTexture(g_dev, &li);
    if (!g_lienzo) {
        fprintf(stderr, "[gpu2d] no pude crear el lienzo: %s\n", SDL_GetError());
        return 1;
    }
    g_ancho_int = ai; g_alto_int = hi; g_escala = escala;
    return 0;
}

static SDL_GPUGraphicsPipeline *crear_pipeline(void)
{
    SDL_GPUShader *vs = cargar_shader("sprite2d.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
    SDL_GPUShader *fs = cargar_shader("sprite2d.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);
    if (!vs || !fs) return NULL;

    SDL_GPUVertexBufferDescription vbd = {
        .slot = 0, .pitch = sizeof(Vertice2D), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    };
    SDL_GPUVertexAttribute attrs[3] = {
        { .location = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = 0 },
        { .location = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = 8 },
        // 4 bytes que llegan al shader como vec4 de 0..1: eso es el _NORM.
        { .location = 2, .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, .offset = 16 },
    };
    // Mezcla "normal": lo nuevo encima de lo viejo segun su alfa.
    //   color = nuevo * a + viejo * (1 - a)
    SDL_GPUColorTargetDescription ctd = {
        .format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
        .blend_state = {
            .enable_blend = true,
            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .color_blend_op = SDL_GPU_BLENDOP_ADD,
            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
        },
    };
    SDL_GPUGraphicsPipelineCreateInfo pi = {
        .vertex_shader = vs, .fragment_shader = fs,
        .vertex_input_state = {
            .vertex_buffer_descriptions = &vbd, .num_vertex_buffers = 1,
            .vertex_attributes = attrs, .num_vertex_attributes = 3,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        // Sin culling: un cuadrilatero espejado queda "de espaldas" y
        // igual se tiene que ver.
        .rasterizer_state = { .fill_mode = SDL_GPU_FILLMODE_FILL,
                              .cull_mode = SDL_GPU_CULLMODE_NONE },
        // Sin depth: el orden lo pone el juego (el pintor).
        .target_info = { .color_target_descriptions = &ctd, .num_color_targets = 1 },
    };
    SDL_GPUGraphicsPipeline *p = SDL_CreateGPUGraphicsPipeline(g_dev, &pi);
    if (!p) fprintf(stderr, "[gpu2d] CreateGraphicsPipeline: %s\n", SDL_GetError());
    SDL_ReleaseGPUShader(g_dev, vs);
    SDL_ReleaseGPUShader(g_dev, fs);
    return p;
}

// Los indices de un cuadrilatero siempre son los mismos (0,1,2, 0,2,3 +4k):
// se calculan una vez al arrancar y nunca mas se tocan.
static int crear_buffers(void)
{
    SDL_GPUBufferCreateInfo bv = { .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                   .size = MAX_CUADS * 4 * (Uint32)sizeof(Vertice2D) };
    SDL_GPUBufferCreateInfo bi = { .usage = SDL_GPU_BUFFERUSAGE_INDEX,
                                   .size = MAX_CUADS * 6 * (Uint32)sizeof(Uint32) };
    g_vbuf = SDL_CreateGPUBuffer(g_dev, &bv);
    g_ibuf = SDL_CreateGPUBuffer(g_dev, &bi);
    SDL_GPUTransferBufferCreateInfo ti = { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                           .size = bv.size };
    g_subida = SDL_CreateGPUTransferBuffer(g_dev, &ti);
    if (!g_vbuf || !g_ibuf || !g_subida) return 1;

    SDL_GPUTransferBufferCreateInfo tii = { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                            .size = bi.size };
    SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(g_dev, &tii);
    Uint32 *idx = SDL_MapGPUTransferBuffer(g_dev, tb, false);
    for (Uint32 q = 0; q < MAX_CUADS; q++) {
        Uint32 b = q * 4;
        idx[q*6+0] = b; idx[q*6+1] = b + 1; idx[q*6+2] = b + 2;
        idx[q*6+3] = b; idx[q*6+4] = b + 2; idx[q*6+5] = b + 3;
    }
    SDL_UnmapGPUTransferBuffer(g_dev, tb);
    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(g_dev);
    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation src = { .transfer_buffer = tb };
    SDL_GPUBufferRegion dst = { .buffer = g_ibuf, .size = bi.size };
    SDL_UploadToGPUBuffer(cp, &src, &dst, false);
    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(g_dev, tb);

    g_verts = SDL_malloc(MAX_CUADS * 4 * sizeof(Vertice2D));
    return g_verts ? 0 : 1;
}

// ── Ciclo de vida ───────────────────────────────────────────
static int gpu2d_init(const char *titulo, int aw, int ah, int alto_objetivo)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "[gpu2d] SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    g_dev = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
    if (!g_dev) {
        fprintf(stderr, "[gpu2d] CreateGPUDevice: %s\n", SDL_GetError());
        fprintf(stderr, "[gpu2d] falta un driver Vulkan (ICD). Ver plugins/renderer3d/README.md\n");
        return 1;
    }
    g_win = SDL_CreateWindow(titulo, aw, ah, SDL_WINDOW_RESIZABLE);
    if (!g_win || !SDL_ClaimWindowForGPUDevice(g_dev, g_win)) {
        fprintf(stderr, "[gpu2d] ventana/swapchain: %s\n", SDL_GetError());
        return 1;
    }
    g_alto_objetivo = alto_objetivo > 0 ? alto_objetivo : 480;
    if (rehacer_lienzo()) return 1;

    SDL_GPUSamplerCreateInfo si = {
        .min_filter = SDL_GPU_FILTER_NEAREST, .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };
    g_sampler = SDL_CreateGPUSampler(g_dev, &si);
    g_pipe = crear_pipeline();
    if (!g_sampler || !g_pipe || crear_buffers()) return 1;

    uint32_t blanco = BLANCO2;
    g_blanca = subir_textura(&blanco, 1, 1);
    return g_blanca == TEXTURA2D_NULA;
}

static void gpu2d_shutdown(void)
{
    if (!g_dev) return;
    SDL_WaitForGPUIdle(g_dev);
    for (Uint32 i = 0; i < g_n_texturas; i++) SDL_ReleaseGPUTexture(g_dev, g_texturas[i].tex);
    g_n_texturas = 0;
    if (g_subida)  SDL_ReleaseGPUTransferBuffer(g_dev, g_subida);
    if (g_vbuf)    SDL_ReleaseGPUBuffer(g_dev, g_vbuf);
    if (g_ibuf)    SDL_ReleaseGPUBuffer(g_dev, g_ibuf);
    if (g_sampler) SDL_ReleaseGPUSampler(g_dev, g_sampler);
    if (g_lienzo)  SDL_ReleaseGPUTexture(g_dev, g_lienzo);
    if (g_pipe)    SDL_ReleaseGPUGraphicsPipeline(g_dev, g_pipe);
    if (g_win)     SDL_ReleaseWindowFromGPUDevice(g_dev, g_win);
    SDL_DestroyGPUDevice(g_dev);
    if (g_win) SDL_DestroyWindow(g_win);
    SDL_free(g_verts);
    g_verts = NULL; g_dev = NULL; g_win = NULL;
    SDL_Quit();
}

// ── Recursos ────────────────────────────────────────────────
static Textura2D gpu2d_textura_desde_pixeles(const uint32_t *p, int w, int h)
{
    if (!p || w <= 0 || h <= 0) return TEXTURA2D_NULA;
    return subir_textura(p, w, h);
}

// El PNG se convierte a ARGB8888: asi da igual si el archivo venia con
// paleta, en RGB o con alfa. Una sola forma adentro del motor. Las filas se
// copian a un bloque compacto porque una surface puede traer relleno al
// final de cada fila (pitch > ancho * 4).
static uint32_t *gpu2d_imagen_cargar(const char *ruta, int *w, int *h)
{
    SDL_Surface *s = IMG_Load(ruta);
    if (!s) {
        fprintf(stderr, "[gpu2d] no pude cargar %s: %s\n", ruta, SDL_GetError());
        return NULL;
    }
    SDL_Surface *c = SDL_ConvertSurface(s, SDL_PIXELFORMAT_ARGB8888);
    SDL_DestroySurface(s);
    if (!c) return NULL;

    uint32_t *pix = SDL_malloc((size_t)c->w * (size_t)c->h * 4);
    if (pix) {
        for (int y = 0; y < c->h; y++)
            memcpy(pix + (size_t)y * c->w, (Uint8 *)c->pixels + (size_t)y * c->pitch,
                   (size_t)c->w * 4);
        if (w) *w = c->w;
        if (h) *h = c->h;
    }
    SDL_DestroySurface(c);
    return pix;
}

static void gpu2d_imagen_liberar(uint32_t *pix) { SDL_free(pix); }

static Textura2D gpu2d_textura_cargar(const char *ruta)
{
    int w = 0, h = 0;
    uint32_t *pix = gpu2d_imagen_cargar(ruta, &w, &h);
    if (!pix) return TEXTURA2D_NULA;
    Textura2D t = subir_textura(pix, w, h);
    SDL_free(pix);
    return t;
}

static void gpu2d_textura_tamano(Textura2D t, int *w, int *h)
{
    int ok = t != TEXTURA2D_NULA && t <= g_n_texturas;
    if (w) *w = ok ? g_texturas[t - 1].w : 0;
    if (h) *h = ok ? g_texturas[t - 1].h : 0;
}

// ── Anotar ──────────────────────────────────────────────────

// Devuelve los 4 vertices libres del proximo cuadrilatero, abriendo un lote
// nuevo si la textura cambio. NULL si el frame se lleno.
static Vertice2D *reservar(Textura2D t)
{
    if (g_n_cuads >= MAX_CUADS) {
        if (!g_avisado_lleno) fprintf(stderr, "[gpu2d] mas de %d cuadrilateros en un frame\n", MAX_CUADS);
        g_avisado_lleno = 1;
        return NULL;
    }
    Lote *ult = g_n_lotes ? &g_lotes[g_n_lotes - 1] : NULL;
    if (!ult || ult->tex != t) {
        if (g_n_lotes >= MAX_LOTES) return NULL;
        g_lotes[g_n_lotes++] = (Lote){ t, g_n_cuads, 0 };
        ult = &g_lotes[g_n_lotes - 1];
    }
    ult->cantidad++;
    return &g_verts[(g_n_cuads++) * 4];
}

static void gpu2d_frame_begin(uint32_t fondo)
{
    g_n_cuads = 0; g_n_lotes = 0;
    g_cam_x = g_cam_y = 0;
    g_fondo = fondo;
}

static void gpu2d_camara(float x, float y) { g_cam_x = x; g_cam_y = y; }

static void gpu2d_sprite(Textura2D t, Rect2D o, Rect2D d, uint32_t tinte, int espejo)
{
    if (t == TEXTURA2D_NULA || t > g_n_texturas) return;
    Vertice2D *v = reservar(t);
    if (!v) return;

    float tw = (float)g_texturas[t - 1].w, th = (float)g_texturas[t - 1].h;
    float u0 = o.x / tw, u1 = (o.x + o.w) / tw;
    float v0 = o.y / th, v1 = (o.y + o.h) / th;
    if (espejo) { float tmp = u0; u0 = u1; u1 = tmp; }

    float x0 = d.x - g_cam_x, y0 = d.y - g_cam_y;
    float x1 = x0 + d.w,      y1 = y0 + d.h;
    uint32_t c = argb_a_rgba(tinte);
    v[0] = (Vertice2D){ x0, y0, u0, v0, c };
    v[1] = (Vertice2D){ x1, y0, u1, v0, c };
    v[2] = (Vertice2D){ x1, y1, u1, v1, c };
    v[3] = (Vertice2D){ x0, y1, u0, v1, c };
}

static void gpu2d_rect(Rect2D r, uint32_t argb)
{
    gpu2d_sprite(g_blanca, (Rect2D){ 0, 0, 1, 1 }, r, argb, 0);
}

static void gpu2d_cuad(const V2 p[4], const uint32_t argb[4])
{
    Vertice2D *v = reservar(g_blanca);
    if (!v) return;
    for (int i = 0; i < 4; i++)
        v[i] = (Vertice2D){ p[i].x - g_cam_x, p[i].y - g_cam_y, 0.5f, 0.5f,
                            argb_a_rgba(argb[i]) };
}

// ── Presentar ───────────────────────────────────────────────
static void gpu2d_frame_end(void)
{
    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(g_dev);
    if (!cmd) return;

    // 1. Los vertices del frame a la GPU. cycle=true: si la GPU todavia esta
    // leyendo el buffer del frame anterior, SDL usa otro por debajo en vez
    // de esperar. Es lo que permite escribir el mismo buffer cada frame.
    if (g_n_cuads > 0) {
        Uint32 bytes = g_n_cuads * 4 * (Uint32)sizeof(Vertice2D);
        void *mapa = SDL_MapGPUTransferBuffer(g_dev, g_subida, true);
        memcpy(mapa, g_verts, bytes);
        SDL_UnmapGPUTransferBuffer(g_dev, g_subida);
        SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmd);
        SDL_GPUTransferBufferLocation src = { .transfer_buffer = g_subida };
        SDL_GPUBufferRegion dst = { .buffer = g_vbuf, .size = bytes };
        SDL_UploadToGPUBuffer(cp, &src, &dst, true);
        SDL_EndGPUCopyPass(cp);
    }

    // 2. Los lotes, en orden, sobre el lienzo limpio.
    SDL_FColor fondo = {
        (float)((g_fondo >> 16) & 0xFF) / 255.0f, (float)((g_fondo >> 8) & 0xFF) / 255.0f,
        (float)(g_fondo & 0xFF) / 255.0f, 1.0f
    };
    SDL_GPUColorTargetInfo ct = {
        .texture = g_lienzo, .clear_color = fondo,
        .load_op = SDL_GPU_LOADOP_CLEAR, .store_op = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmd, &ct, 1, NULL);
    if (pass && g_n_cuads > 0) {
        SDL_BindGPUGraphicsPipeline(pass, g_pipe);
        SDL_GPUBufferBinding vb = { .buffer = g_vbuf };
        SDL_GPUBufferBinding ib = { .buffer = g_ibuf };
        SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
        SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        ULienzo ul = { { (float)g_ancho_int, (float)g_alto_int, 0, 0 } };
        SDL_PushGPUVertexUniformData(cmd, 0, &ul, sizeof(ul));
        for (Uint32 i = 0; i < g_n_lotes; i++) {
            Lote *l = &g_lotes[i];
            SDL_GPUTextureSamplerBinding ts = { .texture = g_texturas[l->tex - 1].tex,
                                                .sampler = g_sampler };
            SDL_BindGPUFragmentSamplers(pass, 0, &ts, 1);
            SDL_DrawGPUIndexedPrimitives(pass, l->cantidad * 6, 1, l->primero * 6, 0, 0);
        }
    }
    if (pass) SDL_EndGPURenderPass(pass);

    // 3. El lienzo a la ventana, por un factor entero y centrado. Lo que
    // sobra en los bordes (menos de un pixel de arte) queda del color de
    // fondo: por eso CLEAR y no DONT_CARE.
    SDL_GPUTexture *sw = NULL;
    Uint32 sw_w = 0, sw_h = 0;
    if (SDL_WaitAndAcquireGPUSwapchainTexture(cmd, g_win, &sw, &sw_w, &sw_h) && sw) {
        Uint32 dw = (Uint32)(g_ancho_int * g_escala), dh = (Uint32)(g_alto_int * g_escala);
        if (dw > sw_w) dw = sw_w;
        if (dh > sw_h) dh = sw_h;
        SDL_GPUBlitInfo bi = {
            .source = { .texture = g_lienzo, .w = (Uint32)g_ancho_int, .h = (Uint32)g_alto_int },
            .destination = { .texture = sw, .x = (sw_w - dw) / 2, .y = (sw_h - dh) / 2,
                             .w = dw, .h = dh },
            .load_op = SDL_GPU_LOADOP_CLEAR, .clear_color = fondo,
            .filter = SDL_GPU_FILTER_NEAREST,
        };
        SDL_BlitGPUTexture(cmd, &bi);
    }
    SDL_SubmitGPUCommandBuffer(cmd);
}

// ── Plataforma ──────────────────────────────────────────────
static int gpu2d_poll_quit(void)
{
    SDL_Event e; int salir = 0;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) salir = 1;
        else if (e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) rehacer_lienzo();
    }
    return salir;
}

static SDL_Scancode scancode_de(Tecla2D k)
{
    if (k >= T2_A && k <= T2_Z) return (SDL_Scancode)(SDL_SCANCODE_A + (k - T2_A));
    // En SDL el 1 va antes que el 0 (asi esta en el teclado).
    if (k == T2_0) return SDL_SCANCODE_0;
    if (k >= T2_1 && k <= T2_9) return (SDL_Scancode)(SDL_SCANCODE_1 + (k - T2_1));
    if (k >= T2_F1 && k <= T2_F4) return (SDL_Scancode)(SDL_SCANCODE_F1 + (k - T2_F1));
    switch (k) {
    case T2_ESCAPE: return SDL_SCANCODE_ESCAPE;
    case T2_SPACE:  return SDL_SCANCODE_SPACE;
    case T2_SHIFT:  return SDL_SCANCODE_LSHIFT;
    case T2_CTRL:   return SDL_SCANCODE_LCTRL;
    case T2_TAB:    return SDL_SCANCODE_TAB;
    case T2_ENTER:  return SDL_SCANCODE_RETURN;
    case T2_UP:     return SDL_SCANCODE_UP;
    case T2_DOWN:   return SDL_SCANCODE_DOWN;
    case T2_LEFT:   return SDL_SCANCODE_LEFT;
    case T2_RIGHT:  return SDL_SCANCODE_RIGHT;
    default:        return SDL_SCANCODE_UNKNOWN;
    }
}

static int gpu2d_tecla(Tecla2D k)
{
    if (k < 0 || k >= T2_COUNT) return 0;
    const bool *st = SDL_GetKeyboardState(NULL);
    // Shift y Ctrl valen por cualquiera de los dos lados.
    if (k == T2_SHIFT) return st[SDL_SCANCODE_LSHIFT] || st[SDL_SCANCODE_RSHIFT];
    if (k == T2_CTRL)  return st[SDL_SCANCODE_LCTRL] || st[SDL_SCANCODE_RCTRL];
    return st[scancode_de(k)] ? 1 : 0;
}

// SDL da el mouse en coordenadas LOGICAS de la ventana. Se pasan a pixeles
// reales (densidad), se descuenta el margen del centrado y se divide por la
// escala: queda el pixel del lienzo.
static void gpu2d_mouse_pos(float *x, float *y)
{
    float mx = 0, my = 0;
    SDL_GetMouseState(&mx, &my);
    float dens = SDL_GetWindowPixelDensity(g_win);
    int pw = 0, ph = 0;
    SDL_GetWindowSizeInPixels(g_win, &pw, &ph);
    float ox = (float)(pw - g_ancho_int * g_escala) / 2.0f;
    float oy = (float)(ph - g_alto_int * g_escala) / 2.0f;
    if (x) *x = (mx * dens - ox) / (float)g_escala;
    if (y) *y = (my * dens - oy) / (float)g_escala;
}

static int gpu2d_mouse_boton(Boton2D b)
{
    if (b < 0 || b >= M2_COUNT) return 0;
    static const int sdl_btn[M2_COUNT] = { SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT, SDL_BUTTON_MIDDLE };
    return (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(sdl_btn[b])) ? 1 : 0;
}

static int gpu2d_mouse_dentro(void)
{
    return g_win && SDL_GetMouseFocus() == g_win;
}

static uint32_t gpu2d_ticks(void)        { return (uint32_t)SDL_GetTicks(); }
static void     gpu2d_delay(uint32_t ms) { SDL_Delay(ms); }
static int      gpu2d_ancho(void)        { return g_ancho_int; }
static int      gpu2d_alto(void)         { return g_alto_int; }
static int      gpu2d_escala(void)       { return g_escala; }

// ── Verificar sin pantalla ──────────────────────────────────
int gpu2d_sdl_capturar_bmp(const char *ruta)
{
    if (!g_dev || !g_lienzo) return 1;
    Uint32 bytes = (Uint32)(g_ancho_int * g_alto_int * 4);
    SDL_GPUTransferBufferCreateInfo ti = { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
                                           .size = bytes };
    SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(g_dev, &ti);
    if (!tb) return 1;

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(g_dev);
    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureRegion src = { .texture = g_lienzo, .w = (Uint32)g_ancho_int,
                                 .h = (Uint32)g_alto_int, .d = 1 };
    SDL_GPUTextureTransferInfo dst = { .transfer_buffer = tb,
                                       .pixels_per_row = (Uint32)g_ancho_int,
                                       .rows_per_layer = (Uint32)g_alto_int };
    SDL_DownloadFromGPUTexture(cp, &src, &dst);
    SDL_EndGPUCopyPass(cp);
    // submit no espera: sin la fence se leeria el buffer antes de tiempo.
    SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    SDL_WaitForGPUFences(g_dev, true, &fence, 1);
    SDL_ReleaseGPUFence(g_dev, fence);

    int fallo = 1;
    void *pix = SDL_MapGPUTransferBuffer(g_dev, tb, false);
    if (pix) {
        SDL_Surface *sup = SDL_CreateSurfaceFrom(g_ancho_int, g_alto_int,
                                                 SDL_PIXELFORMAT_ARGB8888, pix, g_ancho_int * 4);
        if (sup) { fallo = SDL_SaveBMP(sup, ruta) ? 0 : 1; SDL_DestroySurface(sup); }
        SDL_UnmapGPUTransferBuffer(g_dev, tb);
    }
    SDL_ReleaseGPUTransferBuffer(g_dev, tb);
    if (fallo) fprintf(stderr, "[gpu2d] captura fallo: %s\n", SDL_GetError());
    return fallo;
}

// ── El vtable: lo unico publico ─────────────────────────────
Renderer2D gpu2d_sdl_renderer2d = {
    .init                  = gpu2d_init,
    .shutdown              = gpu2d_shutdown,
    .textura_desde_pixeles = gpu2d_textura_desde_pixeles,
    .textura_cargar        = gpu2d_textura_cargar,
    .textura_tamano        = gpu2d_textura_tamano,
    .imagen_cargar         = gpu2d_imagen_cargar,
    .imagen_liberar        = gpu2d_imagen_liberar,
    .frame_begin           = gpu2d_frame_begin,
    .camara                = gpu2d_camara,
    .sprite                = gpu2d_sprite,
    .rect                  = gpu2d_rect,
    .cuad                  = gpu2d_cuad,
    .frame_end             = gpu2d_frame_end,
    .poll_quit             = gpu2d_poll_quit,
    .tecla                 = gpu2d_tecla,
    .mouse_pos             = gpu2d_mouse_pos,
    .mouse_boton           = gpu2d_mouse_boton,
    .mouse_dentro          = gpu2d_mouse_dentro,
    .ticks_ms              = gpu2d_ticks,
    .delay_ms              = gpu2d_delay,
    .ancho_interno         = gpu2d_ancho,
    .alto_interno          = gpu2d_alto,
    .escala                = gpu2d_escala,
};
