#!/bin/bash
# ============================================================
# FASE 2 - Ejercicio 2.4: Backup automatico
# ============================================================
# OBJETIVO: Crear un sistema de backup robusto con validacion
#
# REQUISITOS:
# 1. Recibir: directorio_origen y directorio_destino
# 2. Validar que ambos existen
# 3. Crear tar.gz con formato: backup_YYYYMMDD_HHMMSS.tar.gz
# 4. Mostrar progreso
# 5. Verificar integridad del archivo
# 6. Si hay mas de 5 backups, borrar el mas viejo
# 7. Generar log con fecha, tamanio, archivos, tiempo
# 8. Aceptar --dry-run
#
# USO: ./ejercicio_2.4_backup.sh /home/usuario /backups [--dry-run]
# ============================================================

# Colores
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

DRY_RUN=false
LOG_FILE="/var/log/backup_script.log"

usage() {
    echo "Uso: $0 <directorio_origen> <directorio_destino> [--dry-run]"
    exit 1
}

log_msg() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOG_FILE"
}

# Parsear argumentos
if [ $# -lt 2 ]; then
    usage
fi

ORIGEN="$1"
DESTINO="$2"

if [ "$3" = "--dry-run" ]; then
    DRY_RUN=true
    echo -e "${YELLOW}=== MODO DRY-RUN: No se ejecutara nada ===${NC}"
fi

# Validaciones
if [ ! -d "$ORIGEN" ]; then
    echo -e "${RED}ERROR: Directorio origen '$ORIGEN' no existe${NC}"
    exit 1
fi

if [ ! -d "$DESTINO" ]; then
    echo -e "${RED}ERROR: Directorio destino '$DESTINO' no existe${NC}"
    exit 1
fi

# Variables
TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
BACKUP_NAME="backup_${TIMESTAMP}.tar.gz"
BACKUP_PATH="${DESTINO}/${BACKUP_NAME}"
FILE_COUNT=$(find "$ORIGEN" -type f | wc -l)

echo "========================================="
echo "  BACKUP"
echo "========================================="
echo "Origen   : $ORIGEN"
echo "Destino  : $BACKUP_PATH"
echo "Archivos : $FILE_COUNT"
echo "========================================="

if [ "$DRY_RUN" = true ]; then
    echo ""
    echo "Archivos que se incluirian:"
    find "$ORIGEN" -type f | head -20
    TOTAL=$(find "$ORIGEN" -type f | wc -l)
    [ "$TOTAL" -gt 20 ] && echo "... y $((TOTAL - 20)) mas"

    # Verificar backups viejos
    BACKUP_COUNT=$(ls -1 "$DESTINO"/backup_*.tar.gz 2>/dev/null | wc -l)
    if [ "$BACKUP_COUNT" -ge 5 ]; then
        OLDEST=$(ls -1t "$DESTINO"/backup_*.tar.gz | tail -1)
        echo ""
        echo "Se eliminaria el backup mas viejo: $OLDEST"
    fi
    echo ""
    echo -e "${YELLOW}DRY-RUN completado. No se hicieron cambios.${NC}"
    exit 0
fi

# Crear backup
START_TIME=$(date +%s)
echo ""
echo "Comprimiendo..."

tar -czf "$BACKUP_PATH" -C "$(dirname "$ORIGEN")" "$(basename "$ORIGEN")" 2>&1

if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: Fallo al crear el backup${NC}"
    log_msg "ERROR: Fallo backup de $ORIGEN"
    exit 1
fi

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))
BACKUP_SIZE=$(du -h "$BACKUP_PATH" | awk '{print $1}')

# Verificar integridad
echo "Verificando integridad..."
tar -tzf "$BACKUP_PATH" > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Integridad OK${NC}"
else
    echo -e "${RED}ERROR: El archivo esta corrupto${NC}"
    rm -f "$BACKUP_PATH"
    exit 1
fi

# Limpiar backups viejos (mantener solo 5)
BACKUP_COUNT=$(ls -1 "$DESTINO"/backup_*.tar.gz 2>/dev/null | wc -l)
if [ "$BACKUP_COUNT" -gt 5 ]; then
    echo ""
    echo "Limpiando backups viejos (manteniendo 5)..."
    ls -1t "$DESTINO"/backup_*.tar.gz | tail -n +6 | while read -r OLD; do
        echo "  Eliminando: $(basename "$OLD")"
        rm -f "$OLD"
    done
fi

# Log
log_msg "BACKUP OK | Origen: $ORIGEN | Archivo: $BACKUP_NAME | Tamanio: $BACKUP_SIZE | Archivos: $FILE_COUNT | Tiempo: ${DURATION}s"

echo ""
echo "========================================="
echo -e "${GREEN}  BACKUP COMPLETADO${NC}"
echo "========================================="
echo "Archivo  : $BACKUP_NAME"
echo "Tamanio  : $BACKUP_SIZE"
echo "Archivos : $FILE_COUNT"
echo "Tiempo   : ${DURATION} segundos"
echo "========================================="
