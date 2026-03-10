#ifndef PROGRESO_H
#define PROGRESO_H

int cargar_progreso(const char *path);
int guardar_progreso(const char *path);
int esta_completado(int numero);
int marcar_completado(int numero);
int nivel_desbloqueado(int numero);
int total_completados(void);
int resetear_progreso(void);

#endif
