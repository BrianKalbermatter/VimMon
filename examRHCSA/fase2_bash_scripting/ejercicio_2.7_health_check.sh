#!/bin/bash
# ============================================================
# FASE 2 - Ejercicio 2.7: Health Check con arrays y funciones
# ============================================================
# OBJETIVO: Practicar arrays, funciones, colores y generacion de reportes
#
# REQUISITOS:
# 1. Array de servidores/hosts
# 2. Array de puertos a revisar
# 3. Funciones separadas para cada check
# 4. Colores: verde OK, rojo FAIL
# 5. Reporte HTML
#
# USO: ./ejercicio_2.7_health_check.sh
# ============================================================

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Configuracion - MODIFICA ESTOS VALORES
HOSTS=("127.0.0.1" "8.8.8.8" "1.1.1.1")
PORTS=(22 80 443)
DISK_THRESHOLD=80
MEM_THRESHOLD=90
REPORT_FILE="/tmp/health_report_$(date +%Y%m%d_%H%M%S).html"

# Contadores
PASS=0
FAIL=0

ok() {
    echo -e "  ${GREEN}[OK]${NC} $1"
    ((PASS++))
}

fail() {
    echo -e "  ${RED}[FAIL]${NC} $1"
    ((FAIL++))
}

check_ping() {
    local host="$1"
    if ping -c 1 -W 2 "$host" &>/dev/null; then
        ok "Ping a $host"
        return 0
    else
        fail "Ping a $host"
        return 1
    fi
}

check_port() {
    local host="$1"
    local port="$2"
    if timeout 3 bash -c "echo >/dev/tcp/$host/$port" 2>/dev/null; then
        ok "Puerto $port en $host"
        return 0
    else
        fail "Puerto $port en $host"
        return 1
    fi
}

check_disk() {
    local usage
    usage=$(df / | awk 'NR==2 {gsub(/%/,""); print $5}')
    if [ "$usage" -lt "$DISK_THRESHOLD" ]; then
        ok "Disco: ${usage}% usado (umbral: ${DISK_THRESHOLD}%)"
    else
        fail "Disco: ${usage}% usado (umbral: ${DISK_THRESHOLD}%)"
    fi
}

check_memory() {
    local usage
    usage=$(free | awk '/^Mem:/ {printf "%.0f", $3/$2 * 100}')
    if [ "$usage" -lt "$MEM_THRESHOLD" ]; then
        ok "Memoria: ${usage}% usada (umbral: ${MEM_THRESHOLD}%)"
    else
        fail "Memoria: ${usage}% usada (umbral: ${MEM_THRESHOLD}%)"
    fi
}

generate_report() {
    cat << EOF > "$REPORT_FILE"
<!DOCTYPE html>
<html>
<head>
    <title>Health Check Report</title>
    <style>
        body { font-family: monospace; background: #1e1e1e; color: #d4d4d4; padding: 20px; }
        h1 { color: #569cd6; }
        .ok { color: #4ec9b0; }
        .fail { color: #f44747; }
        table { border-collapse: collapse; width: 100%; margin-top: 20px; }
        th, td { border: 1px solid #444; padding: 8px; text-align: left; }
        th { background: #333; }
    </style>
</head>
<body>
    <h1>Health Check Report</h1>
    <p>Fecha: $(date)</p>
    <p>Hostname: $(hostname)</p>

    <h2>Resumen</h2>
    <p class="ok">Pasaron: $PASS</p>
    <p class="fail">Fallaron: $FAIL</p>

    <h2>Checks de red</h2>
    <table>
        <tr><th>Host</th><th>Ping</th><th>Puertos</th></tr>
EOF

    for host in "${HOSTS[@]}"; do
        local ping_status="<span class='fail'>FAIL</span>"
        ping -c 1 -W 2 "$host" &>/dev/null && ping_status="<span class='ok'>OK</span>"

        local port_results=""
        for port in "${PORTS[@]}"; do
            if timeout 3 bash -c "echo >/dev/tcp/$host/$port" 2>/dev/null; then
                port_results+="$port:<span class='ok'>OK</span> "
            else
                port_results+="$port:<span class='fail'>FAIL</span> "
            fi
        done

        echo "        <tr><td>$host</td><td>$ping_status</td><td>$port_results</td></tr>" >> "$REPORT_FILE"
    done

    cat << EOF >> "$REPORT_FILE"
    </table>

    <h2>Sistema local</h2>
    <p>Disco: $(df / | awk 'NR==2 {print $5}') usado</p>
    <p>Memoria: $(free | awk '/^Mem:/ {printf "%.0f%%", $3/$2 * 100}') usada</p>
</body>
</html>
EOF
}

# === EJECUCION ===
echo "========================================="
echo "  HEALTH CHECK - $(date)"
echo "========================================="

# Check hosts
echo ""
echo "--- PING ---"
for host in "${HOSTS[@]}"; do
    check_ping "$host"
done

# Check puertos
echo ""
echo "--- PUERTOS ---"
for host in "${HOSTS[@]}"; do
    for port in "${PORTS[@]}"; do
        check_port "$host" "$port"
    done
done

# Check local
echo ""
echo "--- SISTEMA LOCAL ---"
check_disk
check_memory

# Reporte
echo ""
echo "========================================="
echo "  RESUMEN"
echo "========================================="
echo -e "  ${GREEN}Pasaron : $PASS${NC}"
echo -e "  ${RED}Fallaron: $FAIL${NC}"

generate_report
echo ""
echo "Reporte HTML: $REPORT_FILE"
echo "========================================="
