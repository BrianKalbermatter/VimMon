/*
 * ============================================================
 * FASE 4 - Ejercicio 4.4: Mini Shell
 * ============================================================
 * OBJETIVO: Construir un shell basico combinando todo lo aprendido
 *
 * REQUISITOS:
 * 1. Prompt personalizado
 * 2. Leer y parsear comandos
 * 3. Ejecutar con fork + exec
 * 4. Builtins: cd, pwd, exit, help, history
 * 5. Redireccion basica: comando > archivo
 * 6. Pipe simple: cmd1 | cmd2
 * 7. Ignorar SIGINT en el shell padre
 * 8. Historial de ultimos 10 comandos
 *
 * COMPILAR: gcc -Wall -Wextra -o minishell ejercicio_4.4_minishell.c
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_HISTORY 10

/* Historial circular */
static char history[MAX_HISTORY][MAX_INPUT];
static int history_count = 0;

void add_history(const char *cmd) {
    strncpy(history[history_count % MAX_HISTORY], cmd, MAX_INPUT - 1);
    history[history_count % MAX_HISTORY][MAX_INPUT - 1] = '\0';
    history_count++;
}

void show_history(void) {
    int start = (history_count > MAX_HISTORY) ? history_count - MAX_HISTORY : 0;
    for (int i = start; i < history_count; i++) {
        printf("  %d  %s\n", i + 1, history[i % MAX_HISTORY]);
    }
}

/* Parsear una linea en tokens */
int parse_args(char *line, char **args) {
    int count = 0;
    char *token = strtok(line, " \t\n");

    while (token && count < MAX_ARGS - 1) {
        args[count++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[count] = NULL;
    return count;
}

/* Ejecutar comando con redireccion > */
void exec_with_redirect(char **args) {
    /* Buscar > en los argumentos */
    char *outfile = NULL;
    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], ">") == 0) {
            args[i] = NULL;  /* Terminar argumentos aca */
            outfile = args[i + 1];
            break;
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        /* Hijo: restaurar SIGINT */
        signal(SIGINT, SIG_DFL);

        /* Redireccion si hay archivo */
        if (outfile) {
            int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror(outfile);
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        execvp(args[0], args);
        perror(args[0]);
        exit(127);
    }

    /* Padre: esperar */
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Exit code: %d\n", WEXITSTATUS(status));
    }
}

/* Ejecutar pipe: cmd1 | cmd2 */
void exec_pipe(char *left_cmd, char *right_cmd) {
    char *left_args[MAX_ARGS];
    char *right_args[MAX_ARGS];

    parse_args(left_cmd, left_args);
    parse_args(right_cmd, right_args);

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        /* Primer hijo: escribe al pipe */
        signal(SIGINT, SIG_DFL);
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execvp(left_args[0], left_args);
        perror(left_args[0]);
        exit(127);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        /* Segundo hijo: lee del pipe */
        signal(SIGINT, SIG_DFL);
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        execvp(right_args[0], right_args);
        perror(right_args[0]);
        exit(127);
    }

    /* Padre: cerrar pipe y esperar */
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

/* Comandos builtin */
int handle_builtin(char **args) {
    if (!args[0])
        return 1;

    if (strcmp(args[0], "exit") == 0) {
        printf("Hasta luego!\n");
        exit(0);
    }

    if (strcmp(args[0], "cd") == 0) {
        const char *dir = args[1] ? args[1] : getenv("HOME");
        if (chdir(dir) != 0)
            perror("cd");
        return 1;
    }

    if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)))
            printf("%s\n", cwd);
        else
            perror("pwd");
        return 1;
    }

    if (strcmp(args[0], "help") == 0) {
        printf("=== MINI SHELL ===\n");
        printf("Builtins: cd, pwd, exit, help, history\n");
        printf("Soporta: redireccion (>), pipe (|)\n");
        printf("Ctrl+C no cierra el shell\n");
        return 1;
    }

    if (strcmp(args[0], "history") == 0) {
        show_history();
        return 1;
    }

    return 0; /* No es builtin */
}

int main(void) {
    char input[MAX_INPUT];
    char input_copy[MAX_INPUT];
    char *args[MAX_ARGS];

    /* Ignorar SIGINT en el shell padre */
    signal(SIGINT, SIG_IGN);

    printf("=== MINI SHELL ===\n");
    printf("Escribe 'help' para ver comandos disponibles\n\n");

    while (1) {
        /* Prompt */
        char cwd[512];
        getcwd(cwd, sizeof(cwd));
        char *dir = strrchr(cwd, '/');
        printf("[minishell@%s]$ ", dir ? dir + 1 : cwd);
        fflush(stdout);

        /* Leer input */
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break;
        }

        /* Quitar newline */
        input[strcspn(input, "\n")] = '\0';

        /* Ignorar lineas vacias */
        if (strlen(input) == 0)
            continue;

        /* Guardar en historial */
        add_history(input);

        /* Copiar input antes de parsear (strtok lo modifica) */
        strncpy(input_copy, input, MAX_INPUT);

        /* Verificar si hay pipe */
        char *pipe_pos = strchr(input_copy, '|');
        if (pipe_pos) {
            *pipe_pos = '\0';
            char *left = input_copy;
            char *right = pipe_pos + 1;
            exec_pipe(left, right);
            continue;
        }

        /* Parsear argumentos */
        int argc = parse_args(input, args);
        if (argc == 0)
            continue;

        /* Intentar builtin primero */
        if (handle_builtin(args))
            continue;

        /* Ejecutar comando externo */
        exec_with_redirect(args);
    }

    return 0;
}
