#ifndef NIVELES_H
#define NIVELES_H

#define MAX_NIVELES 32
#define MAX_CASOS 8

typedef struct {
    char datos[256];
    char pregunta[256];
    char esperado[64];
    char tolerancia[64];
} CasoPrueba;

typedef struct {
    int numero;
    char titulo[128];
    char enunciado[2048];
    char pista[1024];
    CasoPrueba casos[MAX_CASOS];
    int num_casos;
} Nivel;

int cargar_niveles(const char *path);
Nivel *obtener_nivel(int numero);
int total_niveles(void);

#endif
