# Ejercicio 5.2: Contribucion Open Source

## Objetivo

Contribuir a un proyecto real open source en C. Esto demuestra
experiencia practica en tu CV y te da visibilidad.

## Proyectos recomendados para empezar

### Nivel principiante
- **htop** (github.com/htop-dev/htop) - Monitor de procesos
- **jq** (github.com/jqlang/jq) - Procesador JSON
- **tmux** (github.com/tmux/tmux) - Terminal multiplexer

### Nivel intermedio
- **Redis** (github.com/redis/redis) - Base de datos en memoria
- **curl** (github.com/curl/curl) - Cliente HTTP
- **SQLite** (sqlite.org) - Base de datos embebida
- **Neovim** (github.com/neovim/neovim) - Editor

### Nivel avanzado
- **Linux Kernel** (kernel.org) - El kernel
- **Git** (github.com/git/git) - Control de versiones
- **FFmpeg** (github.com/FFmpeg/FFmpeg) - Multimedia

## Pasos para contribuir

### 1. Elegir proyecto
- Busca proyectos con tag "good first issue" en GitHub
- Lee el README y CONTRIBUTING.md del proyecto
- Entiende la estructura del codigo

### 2. Configurar entorno
```bash
# Fork en GitHub
# Clonar tu fork
git clone https://github.com/TU_USER/proyecto.git
cd proyecto

# Agregar upstream
git remote add upstream https://github.com/ORIGINAL/proyecto.git

# Crear rama para tu cambio
git checkout -b fix-nombre-descriptivo
```

### 3. Hacer el cambio
- Lee el codigo alrededor del issue
- Haz cambios minimos y enfocados
- Segui el estilo de codigo del proyecto
- Agrega tests si el proyecto los tiene

### 4. Testear
```bash
# Compilar
make

# Correr tests
make test

# Verificar que no rompiste nada
```

### 5. Enviar Pull Request
```bash
git add archivos_modificados
git commit -m "Descripcion clara del cambio"
git push origin fix-nombre-descriptivo
```
- Crea el PR desde GitHub
- Describe que cambiaste y por que
- Referencia el issue que resuelve: "Fixes #123"

### 6. Responder reviews
- Se paciente, puede tardar dias o semanas
- Acepta feedback y ajusta lo que pidan
- No te frustres si te rechazan, es parte del proceso

## Script Bash para automatizar build+test

Crea un script que:
1. Clone el proyecto
2. Instale dependencias
3. Compile
4. Corra los tests
5. Muestre resultado

Esto te sirve como practica de Bash y como herramienta util.

## Tips

- Empieza con issues de documentacion o typos para familiarizarte
- Despues pasa a bugs simples
- Lee el historial de commits para entender el estilo
- Usa las herramientas: gcc -Wall, valgrind, gdb
- No tengas miedo de preguntar en los issues
