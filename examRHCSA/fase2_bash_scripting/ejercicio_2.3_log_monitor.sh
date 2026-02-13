#!/bin/bash
# ============================================================
# FASE 2 - Ejercicio 2.3: Monitor de logs en tiempo real
# ============================================================
# OBJETIVO: Practicar trap, colores, procesamiento de texto en tiempo real
#
# REQUISITOS:
# 1. Recibir un archivo de log como argumento
# 2. Monitorear en tiempo real
# 3. Si detecta "error", "fail" o "critical":
#    - Mostrar en rojo, guardar en archivo de alertas, contar
# 4. Si detecta "warning": mostrar en amarillo
# 5. Al presionar Ctrl+C: mostrar resumen y limpiar
#
# USO: ./ejercicio_2.3_log_monitor.sh /var/log/syslog
# ============================================================

# Colores
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m' # Sin color

# Variables
ALERT_FILE="/tmp/log_alerts_$$.txt"
TOTAL_LINES=0
ALERT_COUNT=0
WARNING_COUNT=0

# Validar argumentos
if [ $# -ne 1 ]; then
    echo "Uso: $0 <archivo_de_log>"
    exit 1
fi

LOG_FILE="$1"

if [ ! -f "$LOG_FILE" ]; then
    echo "ERROR: El archivo '$LOG_FILE' no existe"
    exit 1
fi

# Funcion de limpieza al salir
cleanup() {
    echo ""
    echo "========================================="
    echo "         RESUMEN DE MONITOREO"
    echo "========================================="
    echo "Lineas procesadas : $TOTAL_LINES"
    echo -e "Alertas (error)   : ${RED}$ALERT_COUNT${NC}"
    echo -e "Warnings          : ${YELLOW}$WARNING_COUNT${NC}"
    echo "Archivo de alertas: $ALERT_FILE"
    echo "========================================="

    if [ $ALERT_COUNT -eq 0 ]; then
        rm -f "$ALERT_FILE"
        echo "(Sin alertas, archivo temporal eliminado)"
    fi

    exit 0
}

# Capturar Ctrl+C
trap cleanup SIGINT SIGTERM

echo -e "${GREEN}Monitoreando: $LOG_FILE${NC}"
echo "Presiona Ctrl+C para detener y ver resumen"
echo "========================================="
echo ""

# Monitorear el archivo
tail -f "$LOG_FILE" | while read -r LINE; do
    ((TOTAL_LINES++))

    # Convertir a minusculas para comparar
    LINE_LOWER=$(echo "$LINE" | tr '[:upper:]' '[:lower:]')

    if echo "$LINE_LOWER" | grep -qE "error|fail|critical"; then
        echo -e "${RED}[ALERTA] $LINE${NC}"
        echo "$(date '+%Y-%m-%d %H:%M:%S') - $LINE" >> "$ALERT_FILE"
        ((ALERT_COUNT++))
    elif echo "$LINE_LOWER" | grep -q "warning"; then
        echo -e "${YELLOW}[WARN]   $LINE${NC}"
        ((WARNING_COUNT++))
    else
        echo "         $LINE"
    fi
done
