#ifndef COMBATE_H
#define COMBATE_H

#include "playerC.h"
#include "proyectil.h"

// combate.h — enemigos y el intercambio de disparos.
//
// Vive en su propio archivo porque toca a los DOS bandos. Si el ataque del
// enemigo estuviera en enemigo.c y el del soldado en playerC.c, cada uno
// tendria que incluir al otro y se te arma una dependencia circular.

// Hasta donde llega cada bando. Los dos IGUALES a proposito: con el soldado
// superando al enemigo la pelea se ganaba sola, sin una sola baja. Quedan como
// dos constantes separadas y no como una sola para que puedas desbalancearlas a
// mano el dia que quieras, sin tocar el codigo que las usa.
#define ALCANCE_SOLDADO 12.0f
#define ALCANCE_ENEMIGO 12.0f

// Cuanto se le permite despegarse del puesto. Es lo unico que separa a un
// guardia de un perseguidor: sin este limite el enemigo se iria caminando atras
// del primer soldado que pase.
#define RADIO_GUARDIA 1.0f

// Un enemigo COMPONE un Soldado en vez de repetir sus campos: reusa tal cual el
// movimiento hacia un destino, la posicion en float y la vida. Lo suyo es la
// intencion (que vigila y a quien le tira), no la mecanica de caminar.
//
// El puesto son coordenadas sueltas y no un puntero al tesoro a proposito: asi
// combate.c no necesita saber que existen los tesoros. Podes ponerlo a vigilar
// una puerta, un cuartel o cualquier cosa sin tocar este archivo.
typedef struct {
  Soldado cuerpo;
  float puesto_x, puesto_y; // el lugar que custodia
  int objetivo;             // a quien le esta disparando, -1 si a nadie
} Enemigo;

void enemigo_init(Enemigo *e, float x, float y);

// Le asigna el lugar a custodiar. Hasta que no se lo digas, vigila donde nacio.
void enemigo_vigilar(Enemigo *e, float x, float y);

// Cada enemigo vuelve a su puesto y dispara a lo que se le acerque. NO
// persigue: si el soldado se aleja del alcance, deja de tirar y se queda.
void combate_enemigos(Enemigo *es, int cant_e, Soldado *ss, int cant_s,
                      Proyectil *ps, int cant_p, float dt);

// Los soldados disparan a lo que tengan a tiro. No persiguen: si el enemigo se
// aleja, dejan de tirar.
void combate_soldados(Soldado *ss, int cant_s, Enemigo *es, int cant_e,
                      Proyectil *ps, int cant_p, float dt);

// Avanza las balas y resuelve los impactos contra los dos bandos.
void combate_impactos(Proyectil *ps, int cant_p, Soldado *ss, int cant_s,
                      Enemigo *es, int cant_e);

#endif // COMBATE_H
