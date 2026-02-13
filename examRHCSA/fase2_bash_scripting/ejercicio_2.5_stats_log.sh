#!/bin/bash
# ============================================================
# FASE 2 - Ejercicio 2.5: Procesamiento de texto (logs Apache)
# ============================================================
# OBJETIVO: Dominar grep, awk, sed, sort, uniq para procesar texto
#
# REQUISITOS:
# Dado un archivo access.log con formato Apache, mostrar:
# 1. Las 10 IPs que mas requests hicieron
# 2. Requests por codigo HTTP
# 3. URLs mas visitadas
# 4. GET vs POST
# 5. IPs con mas de 3 errores 401 (posible brute force)
# 6. Generar reporte CSV
#
# USO: ./ejercicio_2.5_stats_log.sh access.log
# ============================================================

# Primero, crear un archivo de log de ejemplo para practicar
generate_sample_log() {
    cat << 'SAMPLE' > /tmp/access_sample.log
192.168.1.1 - - [10/Oct/2024:13:55:36] "GET /index.html HTTP/1.1" 200 2326
192.168.1.2 - - [10/Oct/2024:13:56:01] "POST /login HTTP/1.1" 401 512
192.168.1.1 - - [10/Oct/2024:13:56:15] "GET /about.html HTTP/1.1" 200 1523
192.168.1.3 - - [10/Oct/2024:13:57:02] "GET /index.html HTTP/1.1" 200 2326
192.168.1.2 - - [10/Oct/2024:13:57:30] "POST /login HTTP/1.1" 401 512
192.168.1.4 - - [10/Oct/2024:13:58:01] "GET /products HTTP/1.1" 200 4521
192.168.1.2 - - [10/Oct/2024:13:58:15] "POST /login HTTP/1.1" 401 512
192.168.1.5 - - [10/Oct/2024:13:59:00] "GET /admin HTTP/1.1" 403 289
192.168.1.2 - - [10/Oct/2024:13:59:30] "POST /login HTTP/1.1" 401 512
192.168.1.1 - - [10/Oct/2024:14:00:01] "GET /contact HTTP/1.1" 200 1856
192.168.1.6 - - [10/Oct/2024:14:00:30] "GET /noexiste HTTP/1.1" 404 196
192.168.1.3 - - [10/Oct/2024:14:01:00] "POST /api/data HTTP/1.1" 500 0
192.168.1.1 - - [10/Oct/2024:14:01:30] "GET /index.html HTTP/1.1" 200 2326
192.168.1.7 - - [10/Oct/2024:14:02:00] "GET /index.html HTTP/1.1" 200 2326
192.168.1.2 - - [10/Oct/2024:14:02:30] "POST /login HTTP/1.1" 200 1024
SAMPLE
    echo "Archivo de ejemplo creado en /tmp/access_sample.log"
}

# Si no hay argumento, generar ejemplo
if [ $# -ne 1 ]; then
    echo "Uso: $0 <archivo_access.log>"
    echo ""
    echo "Generando archivo de ejemplo..."
    generate_sample_log
    LOG_FILE="/tmp/access_sample.log"
else
    LOG_FILE="$1"
fi

if [ ! -f "$LOG_FILE" ]; then
    echo "ERROR: Archivo '$LOG_FILE' no encontrado"
    exit 1
fi

echo "========================================="
echo "  ESTADISTICAS DE LOG: $(basename "$LOG_FILE")"
echo "========================================="

# 1. Top 10 IPs
echo ""
echo "--- TOP 10 IPs CON MAS REQUESTS ---"
awk '{print $1}' "$LOG_FILE" | sort | uniq -c | sort -rn | head -10

# 2. Requests por codigo HTTP
echo ""
echo "--- REQUESTS POR CODIGO HTTP ---"
awk '{print $9}' "$LOG_FILE" | sort | uniq -c | sort -rn

# 3. URLs mas visitadas
echo ""
echo "--- URLs MAS VISITADAS ---"
awk '{print $7}' "$LOG_FILE" | sort | uniq -c | sort -rn | head -10

# 4. GET vs POST
echo ""
echo "--- GET vs POST ---"
awk '{gsub(/"/, "", $6); print $6}' "$LOG_FILE" | sort | uniq -c | sort -rn

# 5. IPs con mas de 3 errores 401
echo ""
echo "--- POSIBLE BRUTE FORCE (>3 errores 401) ---"
awk '$9 == 401 {print $1}' "$LOG_FILE" | sort | uniq -c | sort -rn | awk '$1 > 3 {print $1, "intentos fallidos desde", $2}'

BRUTE_COUNT=$(awk '$9 == 401 {print $1}' "$LOG_FILE" | sort | uniq -c | awk '$1 > 3' | wc -l)
if [ "$BRUTE_COUNT" -eq 0 ]; then
    echo "  (ninguna IP sospechosa detectada)"
fi

# 6. Generar CSV
CSV_FILE="/tmp/log_report_$(date +%Y%m%d).csv"
echo "ip,requests,metodo,codigo,url" > "$CSV_FILE"
awk '{gsub(/"/, ""); print $1","$6","$9","$7}' "$LOG_FILE" >> "$CSV_FILE"

echo ""
echo "--- REPORTE CSV ---"
echo "Guardado en: $CSV_FILE"

echo ""
echo "========================================="
