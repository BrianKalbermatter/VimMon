/*
 * ============================================================
 * FASE 4 - Ejercicio 4.6: Monitor de sistema (lee /proc)
 * ============================================================
 * OBJETIVO: Leer informacion del sistema desde /proc
 *
 * REQUISITOS:
 * 1. Uso de CPU (/proc/stat)
 * 2. Uso de memoria (/proc/meminfo)
 * 3. Lista de procesos (/proc/[pid]/status)
 * 4. Uptime (/proc/uptime)
 * 5. Se actualiza cada 2 segundos
 * 6. Teclas: q=salir, s=ordenar memoria, c=ordenar CPU
 *
 * NOTA: Version simplificada sin ncurses para que compile
 * en cualquier sistema. Si queres ncurses, agrega -lncurses
 *
 * COMPILAR: gcc -Wall -Wextra -o sysmon ejercicio_4.6_sysmon.c
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>

#define MAX_PROCS 256

typedef struct {
    int pid;
    char name[256];
    char state;
    long vmrss; /* Memoria residente en kB */
} ProcInfo;

/* Leer uptime */
void show_uptime(void) {
    FILE *fp = fopen("/proc/uptime", "r");
    if (!fp) return;

    double up, idle;
    fscanf(fp, "%lf %lf", &up, &idle);
    fclose(fp);

    int days = (int)up / 86400;
    int hours = ((int)up % 86400) / 3600;
    int mins = ((int)up % 3600) / 60;

    printf("Uptime: %d dias, %d horas, %d minutos\n", days, hours, mins);
}

/* Leer info de memoria */
void show_memory(void) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return;

    long total = 0, free_mem = 0, available = 0, buffers = 0, cached = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "MemTotal:", 9) == 0)
            sscanf(line, "MemTotal: %ld", &total);
        else if (strncmp(line, "MemFree:", 8) == 0)
            sscanf(line, "MemFree: %ld", &free_mem);
        else if (strncmp(line, "MemAvailable:", 13) == 0)
            sscanf(line, "MemAvailable: %ld", &available);
        else if (strncmp(line, "Buffers:", 8) == 0)
            sscanf(line, "Buffers: %ld", &buffers);
        else if (strncmp(line, "Cached:", 7) == 0)
            sscanf(line, "Cached: %ld", &cached);
    }
    fclose(fp);

    long used = total - available;
    double pct = (total > 0) ? ((double)used / total) * 100.0 : 0;

    printf("\nMemoria:\n");
    printf("  Total     : %ld MB\n", total / 1024);
    printf("  Usada     : %ld MB (%.1f%%)\n", used / 1024, pct);
    printf("  Libre     : %ld MB\n", free_mem / 1024);
    printf("  Disponible: %ld MB\n", available / 1024);
    printf("  Buffers   : %ld MB\n", buffers / 1024);
    printf("  Cached    : %ld MB\n", cached / 1024);
}

/* Leer CPU usage (simplificado) */
void show_cpu(void) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return;

    char name[16];
    long user, nice, system, idle, iowait, irq, softirq;

    fscanf(fp, "%s %ld %ld %ld %ld %ld %ld %ld",
           name, &user, &nice, &system, &idle, &iowait, &irq, &softirq);
    fclose(fp);

    long total = user + nice + system + idle + iowait + irq + softirq;
    long busy = total - idle - iowait;
    double pct = (total > 0) ? ((double)busy / total) * 100.0 : 0;

    printf("\nCPU:\n");
    printf("  Uso total : %.1f%%\n", pct);
    printf("  User      : %ld  System: %ld  Idle: %ld\n", user, system, idle);
}

/* Leer info de un proceso */
int read_proc(int pid, ProcInfo *info) {
    char path[512];
    char line[512];

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    info->pid = pid;
    info->name[0] = '\0';
    info->state = '?';
    info->vmrss = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "Name:", 5) == 0)
            sscanf(line, "Name:\t%255s", info->name);
        else if (strncmp(line, "State:", 6) == 0)
            sscanf(line, "State:\t%c", &info->state);
        else if (strncmp(line, "VmRSS:", 6) == 0)
            sscanf(line, "VmRSS:\t%ld", &info->vmrss);
    }

    fclose(fp);
    return 0;
}

/* Comparar por memoria (para qsort) */
int cmp_memory(const void *a, const void *b) {
    return ((ProcInfo *)b)->vmrss - ((ProcInfo *)a)->vmrss;
}

/* Mostrar lista de procesos */
void show_procs(void) {
    DIR *dir = opendir("/proc");
    if (!dir) return;

    ProcInfo procs[MAX_PROCS];
    int count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) && count < MAX_PROCS) {
        /* Solo directorios numericos (PIDs) */
        if (!isdigit(entry->d_name[0]))
            continue;

        int pid = atoi(entry->d_name);
        if (read_proc(pid, &procs[count]) == 0)
            count++;
    }
    closedir(dir);

    /* Ordenar por memoria */
    qsort(procs, count, sizeof(ProcInfo), cmp_memory);

    printf("\n%-8s %-5s %-20s %s\n", "PID", "STATE", "NAME", "MEM (kB)");
    printf("----------------------------------------------\n");

    int show = (count > 15) ? 15 : count;
    for (int i = 0; i < show; i++) {
        printf("%-8d %-5c %-20s %ld\n",
               procs[i].pid,
               procs[i].state,
               procs[i].name,
               procs[i].vmrss);
    }
    printf("\nTotal procesos: %d (mostrando top 15)\n", count);
}

int main(void) {
    printf("=== SYSMON - Monitor de Sistema ===\n");
    printf("Lee directamente de /proc\n");
    printf("Actualiza cada 2 segundos. Ctrl+C para salir.\n");

    while (1) {
        /* Limpiar pantalla */
        printf("\033[2J\033[H");

        printf("=== SYSMON === %s", __TIME__);
        printf(" (Ctrl+C para salir)\n");
        printf("==============================================\n");

        show_uptime();
        show_cpu();
        show_memory();
        show_procs();

        sleep(2);
    }

    return 0;
}
