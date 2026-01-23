# Job Queue en C - Trabajo del Día

## Tu Misión
Crear un sistema de cola de trabajos (Job Queue) con pool de workers en C puro.

---

## PARTE 1: Conceptos a Estudiar

### 1.1 Threads en C (POSIX Threads)
**Qué estudiar:**
- `pthread_create()` - crear un thread
- `pthread_join()` - esperar a que termine un thread
- `pthread_exit()` - terminar un thread
- Cómo pasar argumentos a threads
- Cómo obtener el valor de retorno

**Recurso:** `man pthreads`, `man pthread_create`

**Ejercicio previo:**
```c
// Crear 3 threads que impriman "Hola desde thread N"
```

### 1.2 Mutex (Exclusión Mutua)
**Qué estudiar:**
- `pthread_mutex_t` - tipo de dato
- `pthread_mutex_init()` - inicializar
- `pthread_mutex_lock()` - bloquear (espera si está ocupado)
- `pthread_mutex_unlock()` - desbloquear
- `pthread_mutex_destroy()` - liberar recursos

**Por qué es necesario:**
Cuando múltiples threads acceden a la misma variable (ej: la cola de jobs), pueden ocurrir "race conditions". El mutex garantiza que solo un thread acceda a la vez.

**Ejercicio previo:**
```c
// 5 threads incrementan un contador compartido 1000 veces cada uno
// Sin mutex: resultado incorrecto (< 5000)
// Con mutex: resultado = 5000
```

### 1.3 Condition Variables
**Qué estudiar:**
- `pthread_cond_t` - tipo de dato
- `pthread_cond_wait()` - esperar señal (libera mutex mientras espera)
- `pthread_cond_signal()` - despertar UN thread esperando
- `pthread_cond_broadcast()` - despertar TODOS los threads esperando

**Por qué es necesario:**
Los workers necesitan "dormirse" cuando no hay jobs y "despertarse" cuando llega uno nuevo. Sin esto, harían busy-waiting (gastar CPU en un loop vacío).

**Patrón típico:**
```c
// Thread que espera:
pthread_mutex_lock(&mutex);
while (cola_vacia) {
    pthread_cond_wait(&cond, &mutex);  // se duerme, libera mutex
}
// aquí hay trabajo, mutex está bloqueado
pthread_mutex_unlock(&mutex);

// Thread que notifica:
pthread_mutex_lock(&mutex);
agregar_a_cola(job);
pthread_cond_signal(&cond);  // despertar un worker
pthread_mutex_unlock(&mutex);
```

### 1.4 Estructuras de Datos
**Qué estudiar:**
- Cola circular (ring buffer) con array
- O lista enlazada simple
- Cómo manejar memoria dinámica (`malloc`, `free`)

---

## PARTE 2: Especificación del Proyecto

### 2.1 Estructura de Archivos
```
/root/job-queue/
├── jobqueue.h      # Definiciones y API pública
├── jobqueue.c      # Implementación
├── main.c          # Programa de prueba
└── Makefile        # Compilación
```

### 2.2 Estructuras de Datos Requeridas

```c
// Estado de un job
typedef enum {
    JOB_PENDING,    // En cola, esperando
    JOB_RUNNING,    // Ejecutándose
    JOB_COMPLETED,  // Terminado
    JOB_CANCELLED   // Cancelado antes de ejecutar
} job_status_t;

// Un trabajo
typedef struct {
    int id;                      // ID único
    void (*function)(void*);     // Función a ejecutar
    void *arg;                   // Argumento para la función
    job_status_t status;         // Estado actual
} job_t;

// La cola de trabajos
typedef struct {
    job_t *jobs;                 // Array de jobs
    int capacity;                // Tamaño máximo
    int count;                   // Jobs actuales en cola
    int head;                    // Índice del próximo a sacar
    int tail;                    // Índice donde insertar
    int next_id;                 // Próximo ID a asignar

    pthread_t *workers;          // Array de threads workers
    int num_workers;             // Cantidad de workers

    pthread_mutex_t mutex;       // Protege la cola
    pthread_cond_t not_empty;    // Señal: hay jobs disponibles

    int shutdown;                // Flag para cerrar
} jobqueue_t;
```

### 2.3 API a Implementar

```c
// Crear queue con N workers y capacidad para M jobs
jobqueue_t* jq_create(int num_workers, int capacity);

// Agregar job a la cola. Retorna ID del job, o -1 si error
int jq_submit(jobqueue_t *jq, void (*func)(void*), void *arg);

// Obtener estado de un job por ID
job_status_t jq_status(jobqueue_t *jq, int job_id);

// Cancelar job pendiente. Retorna 0 si ok, -1 si no se puede
int jq_cancel(jobqueue_t *jq, int job_id);

// Esperar a que terminen todos los jobs y cerrar
void jq_shutdown(jobqueue_t *jq);
```

### 2.4 Lógica del Worker

Cada worker ejecuta este loop:
```
1. Bloquear mutex
2. Mientras (cola vacía Y no hay shutdown):
      - Esperar en condition variable
3. Si hay shutdown y cola vacía:
      - Desbloquear mutex y terminar
4. Sacar job de la cola
5. Marcar job como RUNNING
6. Desbloquear mutex
7. Ejecutar la función del job
8. Bloquear mutex
9. Marcar job como COMPLETED
10. Desbloquear mutex
11. Volver al paso 1
```

---

## PARTE 3: Pasos de Implementación

### Paso 1: Setup (15 min)
- [ ] Crear directorio y archivos vacíos
- [ ] Escribir Makefile básico
- [ ] Verificar que compila (aunque esté vacío)

### Paso 2: Estructuras (30 min)
- [ ] Definir enums y structs en `jobqueue.h`
- [ ] Declarar prototipos de funciones

### Paso 3: jq_create (45 min)
- [ ] Asignar memoria para la estructura
- [ ] Asignar memoria para array de jobs
- [ ] Inicializar mutex y condition variable
- [ ] Crear los worker threads
- [ ] Cada worker llama a una función `worker_loop`

### Paso 4: worker_loop (1 hora)
- [ ] Implementar el loop del worker (ver 2.4)
- [ ] Probar con prints que los workers arrancan

### Paso 5: jq_submit (30 min)
- [ ] Bloquear mutex
- [ ] Verificar que hay espacio
- [ ] Crear job con ID único
- [ ] Agregarlo a la cola (manejar índices circulares)
- [ ] Señalar condition variable
- [ ] Desbloquear mutex

### Paso 6: jq_shutdown (30 min)
- [ ] Poner flag shutdown = 1
- [ ] Broadcast a todos los workers
- [ ] Join de todos los threads
- [ ] Liberar memoria

### Paso 7: jq_status y jq_cancel (30 min)
- [ ] Buscar job por ID
- [ ] Retornar estado o cancelar si está PENDING

### Paso 8: Testing (1 hora)
- [ ] Escribir main.c con pruebas
- [ ] Probar con jobs que hacen sleep
- [ ] Verificar ejecución paralela
- [ ] Probar shutdown correcto

---

## PARTE 4: Makefile

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
LDFLAGS = -pthread

TARGET = jobqueue_demo
SRCS = main.c jobqueue.c
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c jobqueue.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
```

---

## PARTE 5: Ejemplo de main.c para Probar

```c
#include <stdio.h>
#include <unistd.h>
#include "jobqueue.h"

void tarea_ejemplo(void *arg) {
    int id = *(int*)arg;
    printf("Job %d: iniciando...\n", id);
    sleep(2);  // simular trabajo
    printf("Job %d: terminado!\n", id);
}

int main() {
    // Crear queue con 3 workers
    jobqueue_t *jq = jq_create(3, 10);

    // Enviar 6 jobs
    int ids[6];
    for (int i = 0; i < 6; i++) {
        ids[i] = i + 1;
        jq_submit(jq, tarea_ejemplo, &ids[i]);
        printf("Enviado job %d\n", i + 1);
    }

    // Esperar un poco y ver estados
    sleep(1);
    for (int i = 0; i < 6; i++) {
        printf("Estado job %d: %d\n", i+1, jq_status(jq, i+1));
    }

    // Cerrar
    printf("Cerrando...\n");
    jq_shutdown(jq);
    printf("Listo!\n");

    return 0;
}
```

**Salida esperada:** Los primeros 3 jobs empiezan inmediatamente (3 workers). Los otros 3 esperan. Todos terminan y el programa cierra limpiamente.

---

## PARTE 6: Errores Comunes

1. **Deadlock**: Olvidar desbloquear mutex antes de return
2. **Race condition**: Acceder a la cola sin mutex
3. **Memory leak**: No liberar jobs o la estructura
4. **Segfault**: Acceder a índices fuera del array
5. **Workers no despiertan**: Olvidar `pthread_cond_signal`
6. **Shutdown no termina**: Workers esperando en cond_wait sin revisar shutdown

---

## PARTE 7: Criterios de Éxito

- [ ] Compila sin warnings con `-Wall -Wextra`
- [ ] Los jobs se ejecutan en paralelo (no secuencial)
- [ ] El número de jobs simultáneos = número de workers
- [ ] `jq_shutdown` termina limpiamente
- [ ] No hay memory leaks (probar con `valgrind`)
- [ ] No hay deadlocks ni race conditions

---

## Recursos

- `man pthread_create`
- `man pthread_mutex_lock`
- `man pthread_cond_wait`
- https://computing.llnl.gov/tutorials/pthreads/

---

¡Buena suerte!
