/*
 * ============================================================
 * FASE 4 - Ejercicio 4.5: Servidor TCP Echo (to uppercase)
 * ============================================================
 * OBJETIVO: Dominar sockets, redes y procesos en C
 *
 * REQUISITOS:
 * 1. Escuchar en un puerto dado como argumento
 * 2. Aceptar conexiones
 * 3. Devolver mensajes en MAYUSCULAS
 * 4. Multiples clientes con fork()
 * 5. Loguear conexiones
 * 6. Cerrar limpiamente con Ctrl+C
 *
 * COMPILAR: gcc -Wall -Wextra -o server ejercicio_4.5_servidor_tcp.c
 * USO:      ./server 9090
 * PROBAR:   nc localhost 9090
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024
#define LOG_FILE "/tmp/tcp_server.log"

static int server_fd = -1;

/* Loguear a archivo */
void log_connection(const char *ip, int port, const char *msg) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) return;

    time_t now = time(NULL);
    char *timestr = ctime(&now);
    timestr[strlen(timestr) - 1] = '\0'; /* Quitar newline */

    fprintf(fp, "[%s] %s:%d - %s\n", timestr, ip, port, msg);
    fclose(fp);
}

/* Convertir a mayusculas */
void to_upper(char *str) {
    for (int i = 0; str[i]; i++)
        str[i] = toupper((unsigned char)str[i]);
}

/* Manejar un cliente (corre en proceso hijo) */
void handle_client(int client_fd, struct sockaddr_in *client_addr) {
    char buffer[BUFFER_SIZE];
    char client_ip[INET_ADDRSTRLEN];
    int client_port = ntohs(client_addr->sin_port);

    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip));

    printf("[HIJO %d] Cliente conectado: %s:%d\n", getpid(), client_ip, client_port);
    log_connection(client_ip, client_port, "CONECTADO");

    /* Enviar saludo */
    const char *welcome = "=== Echo Server (UPPERCASE) ===\nEscribe algo (o 'quit' para salir):\n";
    send(client_fd, welcome, strlen(welcome), 0);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) {
            printf("[HIJO %d] Cliente desconectado: %s:%d\n", getpid(), client_ip, client_port);
            log_connection(client_ip, client_port, "DESCONECTADO");
            break;
        }

        /* Quitar newline */
        buffer[strcspn(buffer, "\r\n")] = '\0';

        /* Comando quit */
        if (strcmp(buffer, "quit") == 0) {
            const char *bye = "Chau!\n";
            send(client_fd, bye, strlen(bye), 0);
            log_connection(client_ip, client_port, "QUIT");
            break;
        }

        /* Loguear mensaje recibido */
        log_connection(client_ip, client_port, buffer);

        /* Convertir y responder */
        to_upper(buffer);
        strcat(buffer, "\n");
        send(client_fd, buffer, strlen(buffer), 0);
    }

    close(client_fd);
    exit(0);
}

/* Limpiar al cerrar con Ctrl+C */
void cleanup(int sig) {
    (void)sig;
    printf("\n[SERVER] Cerrando servidor...\n");
    if (server_fd >= 0)
        close(server_fd);

    /* Matar procesos hijos */
    signal(SIGCHLD, SIG_IGN);
    kill(0, SIGTERM);

    printf("[SERVER] Log guardado en: %s\n", LOG_FILE);
    exit(0);
}

/* Limpiar hijos terminados (evitar zombies) */
void sigchld_handler(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s 9090\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Puerto invalido: %s\n", argv[1]);
        return 1;
    }

    /* Configurar signals */
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGCHLD, sigchld_handler);

    /* Crear socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    /* Reusar puerto */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Bind */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    /* Listen */
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("=== TCP ECHO SERVER ===\n");
    printf("Escuchando en puerto %d\n", port);
    printf("Log: %s\n", LOG_FILE);
    printf("Ctrl+C para detener\n");
    printf("=======================\n\n");

    /* Loop principal: aceptar conexiones */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        /* Fork para manejar el cliente */
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(client_fd);
            continue;
        }

        if (pid == 0) {
            /* Hijo: manejar cliente */
            close(server_fd);
            handle_client(client_fd, &client_addr);
            /* handle_client hace exit() */
        }

        /* Padre: cerrar fd del cliente y seguir aceptando */
        close(client_fd);
    }

    return 0;
}
