#include <ncurses.h>
#include "ui.h"

// En esta funcion solo se INICIALIZAN los colores, se registran todos

void 
ui_init_colors(void)
{
	start_color();
	// La funcion init_pair para cada PAIR_ definido en ui.h
	init_pair(PAIR_TITLE, COLOR_YELLOW, COLOR_BLACK);
	init_pair(PAIR_NORMAL,  COLOR_WHITE,   COLOR_BLACK);
    init_pair(PAIR_GREEN,   COLOR_GREEN,   COLOR_BLACK);
    init_pair(PAIR_RED,     COLOR_RED,     COLOR_BLACK);
    init_pair(PAIR_YELLOW,  COLOR_YELLOW,  COLOR_BLACK);
    init_pair(PAIR_ORANGE,  COLOR_RED,     COLOR_BLACK);
    init_pair(PAIR_GRAY,    COLOR_WHITE,   COLOR_BLACK);
}
// Dibujar
void
mostrar_menu_principal(void)
{
	attron(COLOR_PAIR(PAIR_TITLE)); // Activar para 2 AMARILLO
	mvprintw(2, 5, "PSEUDOGAMES");	// Aca le estoy diciendo 2= pair_title y el 5 es el color pair_yellow
	attroff(COLOR_PAIR(PAIR_TITLE));
	mvprintw(4, 5, "[1] Jugar");
    mvprintw(5, 5, "[2] Wiki");
    mvprintw(6, 5, "[3] Progreso");
    mvprintw(7, 5, "[4] Pomodoro");
    mvprintw(8, 5, "[q] Salir");
}

