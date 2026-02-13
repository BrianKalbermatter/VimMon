#!/bin/bash
# ============================================================
# FASE 2 - Ejercicio 2.1: Script de informacion del sistema
# ============================================================
# OBJETIVO: Crear un script que muestre info del sistema formateada
#
# REQUISITOS:
# - Mostrar: hostname, IP, kernel, uptime, disco, RAM, usuarios, procesos
# - Aceptar flag -s para modo corto (solo hostname, IP y disco)
# - Salida formateada y legible
#
# PISTAS:
# - hostname, ip addr, uname -r, uptime
# - df -h /, free -h, who | wc -l
# - ps aux --sort=-%cpu | head -6
# ============================================================

# TU CODIGO ACA - Implementa el script completo

usage() {
    echo "Uso: $0 [-s]"
    echo "  -s  Modo corto (hostname, IP, disco)"
    exit 1
}

SHORT=false

while getopts "sh" opt; do
    case $opt in
        s) SHORT=true ;;
        h) usage ;;
        *) usage ;;
    esac
done

echo "========================================="
echo "   INFORMACION DEL SISTEMA"
echo "========================================="
echo ""

# Hostname
echo "Hostname    : $(hostname)"

# IP
echo "IP          : $(hostname -I | awk '{print $1}')"

# Disco
echo "Disco /     : $(df -h / | awk 'NR==2 {print $5 " usado de " $2}')"

if [ "$SHORT" = true ]; then
    echo ""
    echo "========================================="
    exit 0
fi

# Kernel
echo "Kernel      : $(uname -r)"

# Uptime
echo "Uptime      : $(uptime -p)"

# RAM
echo "RAM         : $(free -h | awk '/^Mem:/ {print $3 " usada de " $2}')"

# Usuarios logueados
echo "Usuarios    : $(who | wc -l) logueados"

echo ""
echo "--- Top 5 procesos por CPU ---"
echo ""
printf "%-10s %-6s %-6s %s\n" "USER" "%CPU" "%MEM" "COMMAND"
echo "--------------------------------------"
ps aux --sort=-%cpu | awk 'NR>1 && NR<=6 {printf "%-10s %-6s %-6s %s\n", $1, $3, $4, $11}'

echo ""
echo "========================================="
