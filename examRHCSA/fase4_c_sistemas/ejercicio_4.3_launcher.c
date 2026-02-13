/*
 * ============================================================
 * FASE 4 - Ejercicio 4.3: Process Launcher
 * ============================================================
 * OBJETIVO: Dominar fork(), exec(), wait(), signals
 *
 * REQUISITOS:
 * 1. Recibir un comando como argumento
 * 2. fork() para crear proceso hijo
 * 3. El hijo ejecuta con exec()
 * 4. El padre espera con waitpid()
 * 5. Mostrar PID y exit code
 * 6. Timeout de 5 segundos (SIGKILL si se pasa)
 * 7. Manejar SIGCHLD para evitar zombies
 *
 * COMPILAR: gcc -Wall -Wextra -o launcher ejercicio_4.3_launcher.c
 * USO:      ./launcher ls -la
 *           ./launcher sleep 10    (se matara a los 5 seg)
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define TIMEOUT 5

static pid_t child_pid = -1;

/* Manejador de alarma (timeout) */
void alarm_handler(int sig) {
    (void)sig;
    if (child_pid > 0) {
        fprintf(stderr, "\n[TIMEOUT] Proceso hijo %d excedio %d segundos. Matando...\n",
                child_pid, TIMEOUT);
        kill(child_pid, SIGKILL);
    }
}

/* Manejador de SIGCHLD para evitar zombies */
void sigchld_handler(int sig) {
    (void)sig;
    /* Limpiar procesos hijos terminados */
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando> [argumentos...]\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s ls -la /tmp\n", argv[0]);
        return 1;
    }

    /* Configurar manejador de SIGCHLD */
    struct sigaction sa_chld;
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, NULL);

    /* Configurar manejador de alarma */
    struct sigaction sa_alarm;
    sa_alarm.sa_handler = alarm_handler;
    sigemptyset(&sa_alarm.sa_mask);
    sa_alarm.sa_flags = 0;
    sigaction(SIGALRM, &sa_alarm, NULL);

    printf("=== LAUNCHER ===\n");
    printf("Comando: ");
    for (int i = 1; i < argc; i++)
        printf("%s ", argv[i]);
    printf("\n");
    printf("Timeout: %d segundos\n", TIMEOUT);
    printf("================\n\n");

    /* Crear proceso hijo */
    child_pid = fork();

    if (child_pid < 0) {
        perror("fork");
        return 1;
    }

    if (child_pid == 0) {
        /* === PROCESO HIJO === */
        /* argv+1 apunta al comando y sus argumentos */
        execvp(argv[1], argv + 1);

        /* Si exec falla */
        perror(argv[1]);
        exit(127);
    }

    /* === PROCESO PADRE === */
    printf("[PADRE] Proceso hijo creado: PID %d\n\n", child_pid);

    /* Iniciar timer */
    alarm(TIMEOUT);

    /* Esperar al hijo */
    int status;
    pid_t result = waitpid(child_pid, &status, 0);

    /* Cancelar alarma si el hijo termino a tiempo */
    alarm(0);

    printf("\n================\n");

    if (result < 0) {
        if (errno == EINTR) {
            printf("[PADRE] Hijo interrumpido\n");
        } else {
            perror("waitpid");
        }
    } else if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        printf("[PADRE] Hijo %d termino normalmente\n", child_pid);
        printf("[PADRE] Exit code: %d\n", exit_code);
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        printf("[PADRE] Hijo %d terminado por signal: %d (%s)\n",
               child_pid, sig, strsignal(sig));
    }

    return 0;
}
