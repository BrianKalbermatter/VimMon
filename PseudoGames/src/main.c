#include <ncurses.h>
//#include "niveles.h"
//#include "progreso.h"
#include "ui.h"
#include <stdlib.h>
int tecla;
int corriendoMenu = 1;

int main(void) {
	initscr();
	noecho();
	cbreak();
	ui_init_colors();
	while(corriendoMenu) {
		clear(); // Limpia la pantalla
		mostrar_menu_principal();
		//mvprintw(2, 4, "Funciona!"); // Dibuja en coordenada y, x
		refresh(); // Muestra en la pantalla
		tecla = getch(); // Espera una tecla
        
        if(tecla == '4'){
            endwin(); // Devuelve la terminal a su estado normal, cierra ncurses
            system("bash scripts/pomodoro.sh");
            initscr(); // Arranca ncurses, toma control de la terminal
            noecho(); // Lo que tipeas no aparece en pantalla
            cbreak(); // Lee teclas sin esperar Enter
            ui_init_colors(); // Registra tus pares de colores
        }
		
        if (tecla == 'q')
        {
            corriendoMenu = 0;
            clear();
            mvprintw(2, 4, "Hasta Pronto!");
            refresh();
            napms(1500); // Espera 1.5 segundos
	    }
    }
	endwin();
	return 0;
}

