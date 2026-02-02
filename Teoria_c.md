# STRINGS
En C, las cadenas son arreglos de caracteres que terminan con un caracter especial \0 (llamado "null terminator" o terminador nulo). Este caracter le dice a las funciones "aqui termina la cadena".

## Ejemplo:
Cuando escribo:
char nombre[] = "Hola";

En memoria se ve asi:
índice:   [0]  [1]  [2]  [3]  [4]
valor:    'H'  'o'  'l'  'a'  '\0'

El \0 ocupa una posicion extra. Por eso "   Hola" tiene 4 letras pero ocupa 5 bytes.
Como funciona strcat
strcat(destino, fuente)hace esto:

1. Busca el \0 en destino
2. Empieza a copiar fuente desde esa posicion
3. Pone un \0 al final

El problema que a veces se presenta es:
```bash
    char final[50]; // Sin inicializar!
```
En memoria tienes basura aleatoria:
[g] [&] [�] [�] [t] [\0] [?] [?] ...  ← basura
                    ↑
              strcat encuentra el \0 aquí

strcat encontró un \0 aleatorio en posición 5, y puso "Brian" después de la basura.

Con inicialización

char final[50] = "";

[\0] [?] [?] [?] ...  ← vacío, listo para usar
 ↑
strcat empieza aquí

Ahora strcat encuentra el \0 en posición 0 y concatena desde el inicio.              















# Partes de un OS:
    Pensandolo como un juego a desarrollar:
        Juego                |              Sistema Operativo
Esqueleto/hueso              |         BOOTLOADER + Kernel
Organos vitales              |         Gestion de memoria, CPU, interrupciones
Sentidos(Ver, oir)           |         Drivers(teclado, pantalla, disco)
Cerebro                      |         Scheduler (decide que programa corre)
Habilidades                  |         Syscalls (Lo que pueden hacer los programas)
Inventario                   |         Sistema de archivos
Comunicacion                 |         IPC (Como hablan los programas entre si)
Piel/apariencia              |         Shell / Interfaz grafica

Componentes de un SO en orden de desarrollo!

  ┌─────────────────────────────────────────────────────────────┐
  │  7. INTERFAZ DE USUARIO                                     │
  │     Shell (línea de comandos) o GUI                         │
  ├─────────────────────────────────────────────────────────────┤
  │  6. PROGRAMAS DE USUARIO                                    │
  │     Editor, compilador, utilidades                          │
  ├─────────────────────────────────────────────────────────────┤
  │  5. SISTEMA DE ARCHIVOS                                     │
  │     Leer/escribir archivos, directorios                     │
  ├─────────────────────────────────────────────────────────────┤
  │  4. DRIVERS                                                 │
  │     Teclado, pantalla, disco, red                           │
  ├─────────────────────────────────────────────────────────────┤
  │  3. GESTIÓN DE PROCESOS                                     │
  │     Crear procesos, scheduler, multitarea                   │
  ├─────────────────────────────────────────────────────────────┤
  │  2. GESTIÓN DE MEMORIA                                      │
  │     Asignar RAM, memoria virtual, paginación                │
  ├─────────────────────────────────────────────────────────────┤
  │  1. KERNEL BASE                                             │
  │     Interrupciones, GDT/IDT, entrada al modo protegido      │
  ├─────────────────────────────────────────────────────────────┤
  │  0. BOOTLOADER                                              │
  │     Carga el kernel desde disco a memoria                   │
  └─────────────────────────────────────────────────────────────┘
            ▼▼▼ HARDWARE (CPU, RAM, DISCO) ▼▼▼


Detalle de cada componente

  0. Bootloader - "El nacimiento"

  ¿Qué hace?    Arranca la PC, carga el kernel en RAM
  ¿Qué crea?    - Código en sector de arranque (512 bytes)
                - Cambio a modo protegido (32-bit) o long mode (64-bit)
                - Lectura básica de disco

  1. Kernel Base - "El esqueleto"

  ¿Qué hace?    Configura el CPU para que todo funcione
  ¿Qué crea?    - GDT (tabla de segmentos)
                - IDT (tabla de interrupciones)
                - Handlers de excepciones (división por cero, etc.)

  2. Gestión de Memoria - "El corazón"

  ¿Qué hace?    Administra quién usa qué parte de la RAM
  ¿Qué crea?    - Allocator físico (qué páginas están libres)
                - Paginación (memoria virtual)
                - malloc/free para el kernel

  3. Gestión de Procesos - "El cerebro"

  ¿Qué hace?    Ejecuta múltiples programas "a la vez"
  ¿Qué crea?    - Estructura de proceso (PCB)
                - Context switch (cambiar entre procesos)
                - Scheduler (decidir quién corre)

  4. Drivers - "Los sentidos"

  ¿Qué hace?    Comunicación con hardware
  ¿Qué crea?    - Driver de teclado (PS/2 o USB)
                - Driver de pantalla (VGA, framebuffer)
                - Driver de disco (ATA, AHCI, NVMe)
                - Driver de timer (PIT, APIC)

  5. Sistema de Archivos - "La memoria/inventario"

  ¿Qué hace?    Organiza datos en disco
  ¿Qué crea?    - VFS (capa abstracta)
                - Implementación (FAT32, ext2, o tu propio FS)
                - Operaciones: open, read, write, close

  6. Syscalls - "Las habilidades"

  ¿Qué hace?    Permite que programas usen el kernel
  ¿Qué crea?    - Tabla de syscalls
                - Interfaz: write(), read(), fork(), exec(), exit()

  7. Shell - "La voz"

  ¿Qué hace?    El usuario interactúa con el SO
  ¿Qué crea?    - Parser de comandos
                - Comandos básicos: ls, cd, cat, echo


mi-so/
  ├── boot/
  │   └── boot.asm         # Bootloader
  ├── kernel/
  │   ├── main.c           # Punto de entrada
  │   ├── gdt.c            # Tabla de descriptores
  │   ├── idt.c            # Interrupciones
  │   ├── memory.c         # Gestión de memoria
  │   ├── process.c        # Procesos
  │   └── scheduler.c      # Planificador
  ├── drivers/
  │   ├── keyboard.c       # Teclado
  │   ├── screen.c         # Pantalla
  │   └── disk.c           # Disco
  ├── fs/
  │   └── fat32.c          # Sistema de archivos
  ├── lib/
  │   └── string.c         # Funciones auxiliares
  └── Makefile
