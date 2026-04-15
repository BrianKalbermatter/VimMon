#include <stdio.h>
#include <SDL3/SDL.h>
#include "screen1.h"

int
main() {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "Error al inicializar SDL: %s\n", SDL_GetError());
		return 1;
	}

	SDL_Window*   window   = SDL_CreateWindow("*RECTAS*", 800, 800, 0);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
	if (!window || !renderer) {
		fprintf(stderr, "Error al crear la ventana o el renderizador: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	// PAED calcula las coordenadas UNA SOLA VEZ antes del loop
	// C las guarda en cache y las redibuja cada frame
	Comando cmds[MAX_COMANDOS];
	int     n_cmds = 0;
	cargar_paed("recta.paed", cmds, &n_cmds);

	int running = 1;
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT)    running = 0;
			if (event.type == SDL_EVENT_KEY_DOWN)
				if (event.key.key == SDLK_ESCAPE) running = 0;
		}

		// 1. Limpiar con negro
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		// 2. Dibujar lo que PAED calculó
		dibujar_comandos(renderer, cmds, n_cmds);

		// 3. Presentar
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
