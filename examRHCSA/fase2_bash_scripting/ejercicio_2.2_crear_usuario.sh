#!/bin/bash
# ============================================================
# FASE 2 - Ejercicio 2.2: Validador de argumentos / Crear usuario
# ============================================================
# OBJETIVO: Practicar validacion de argumentos, condicionales y logging
#
# REQUISITOS:
# 1. Recibir 3 argumentos: nombre_usuario, grupo, shell
# 2. Validar que:
#    - Se pasaron exactamente 3 argumentos
#    - El usuario no existe ya
#    - El grupo existe (sino crearlo preguntando)
#    - El shell es valido (existe en /etc/shells)
# 3. Crear el usuario con esos parametros
# 4. Mostrar un resumen
# 5. Guardar log en /var/log/crear_usuario.log
#
# USO: sudo ./ejercicio_2.2_crear_usuario.sh nombre grupo /bin/bash
# ============================================================

LOG_FILE="/var/log/crear_usuario.log"

log_msg() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOG_FILE"
}

# Validar cantidad de argumentos
if [ $# -ne 3 ]; then
    echo "Uso: $0 <nombre_usuario> <grupo> <shell>"
    echo "Ejemplo: $0 dev1 desarrollo /bin/bash"
    exit 1
fi

USUARIO="$1"
GRUPO="$2"
SHELL_USER="$3"

# Validar que se ejecuta como root
if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: Este script debe ejecutarse como root"
    exit 1
fi

# Validar que el usuario no existe
if id "$USUARIO" &>/dev/null; then
    echo "ERROR: El usuario '$USUARIO' ya existe"
    exit 1
fi

# Validar que el shell es valido
if ! grep -q "^${SHELL_USER}$" /etc/shells; then
    echo "ERROR: Shell '$SHELL_USER' no es valido"
    echo "Shells disponibles:"
    cat /etc/shells
    exit 1
fi

# Validar que el grupo existe
if ! getent group "$GRUPO" &>/dev/null; then
    echo "El grupo '$GRUPO' no existe."
    read -p "Deseas crearlo? (s/n): " RESPUESTA
    if [ "$RESPUESTA" = "s" ] || [ "$RESPUESTA" = "S" ]; then
        groupadd "$GRUPO"
        log_msg "Grupo '$GRUPO' creado"
    else
        echo "Cancelado."
        exit 1
    fi
fi

# Crear usuario
useradd -g "$GRUPO" -s "$SHELL_USER" "$USUARIO"

if [ $? -eq 0 ]; then
    log_msg "Usuario '$USUARIO' creado - grupo: $GRUPO - shell: $SHELL_USER"
    echo ""
    echo "=== RESUMEN ==="
    echo "Usuario  : $USUARIO"
    echo "UID      : $(id -u "$USUARIO")"
    echo "Grupo    : $GRUPO"
    echo "GID      : $(id -g "$USUARIO")"
    echo "Shell    : $SHELL_USER"
    echo "Home     : /home/$USUARIO"
    echo "==============="
else
    log_msg "ERROR al crear usuario '$USUARIO'"
    echo "ERROR: No se pudo crear el usuario"
    exit 1
fi
