// Delirio solo, sin el editor: el mismo modulo en una ventana a pantalla
// completa. Sirve para jugar y para probar sin interfaz.
//
//   make delirio && ./build/delirio
//   ./build/delirio --semilla 7 --captura /tmp/f.bmp 22000 --ir 0 -2600
//     (simula 22 s con pasos fijos caminando al norte, guarda el frame y sale)

#include "delirio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  const char *captura = NULL;
  int captura_ms = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--semilla") == 0 && i + 1 < argc) {
      delirio_fijar_semilla((unsigned)strtoul(argv[++i], NULL, 10));
    } else if (strcmp(argv[i], "--captura") == 0 && i + 1 < argc) {
      captura = argv[++i];
      if (i + 1 < argc && argv[i + 1][0] != '-') captura_ms = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--ir") == 0 && i + 2 < argc) {
      float dx = strtof(argv[++i], NULL), dy = strtof(argv[++i], NULL);
      delirio_caminar_solo(dx, dy);
    }
  }

  const Modulo2D *m = &delirio_modulo;
  Renderer2D *r = &gpu2d_sdl_renderer2d;
  if (r->init(m->nombre, 1280, 720, m->alto_objetivo) != 0) return 1;
  if (m->cargar(r) != 0) {
    r->shutdown();
    return 1;
  }
  m->iniciar();

  uint32_t antes = r->ticks_ms();
  int simulado = 0;
  while (!r->poll_quit() && !r->tecla(T2_ESCAPE)) {
    float dt;
    if (captura) {
      dt = 16; // pasos fijos: la captura sale igual siempre
    } else {
      uint32_t ahora = r->ticks_ms();
      dt = (float)(ahora - antes);
      if (dt > 100) dt = 100; // una pausa larga no teletransporta
      antes = ahora;
    }
    int W = r->ancho_interno(), H = r->alto_interno();

    Entrada2D e = { 0 };
    r->mouse_pos(&e.mouse_x, &e.mouse_y);
    e.mouse_dentro = !captura && r->mouse_dentro();
    for (int b = 0; b < M2_COUNT; b++) e.boton[b] = r->mouse_boton((Boton2D)b);
    for (int k = 0; k < T2_COUNT; k++) e.tecla[k] = r->tecla((Tecla2D)k);

    m->actualizar(&e, dt, W, H);
    r->frame_begin(RGB2(0, 0, 0));
    m->dibujar(r, W, H);
    r->frame_end();

    if (captura && (simulado += 16) >= captura_ms) {
      int fallo = gpu2d_sdl_capturar_bmp(captura);
      printf("lienzo %dx%d escala %d -> %s\n", W, H, r->escala(), fallo ? "fallo" : captura);
      break;
    }
    if (!captura) r->delay_ms(1);
  }
  m->terminar();
  m->descargar(r);
  r->shutdown();
  return 0;
}
