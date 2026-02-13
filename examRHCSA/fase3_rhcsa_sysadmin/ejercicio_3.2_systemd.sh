#!/bin/bash
# ============================================================
# FASE 3 - Ejercicio 3.2: Systemd - Servicios y Timers
# ============================================================
# OBJETIVO: Crear y gestionar servicios systemd propios
#
# === PARTE 1: Gestionar servicios existentes ===
#
# Verificar estado de sshd
#   systemctl status sshd
#   systemctl is-enabled sshd
#   systemctl is-active sshd
#
# Iniciar y habilitar
#   sudo systemctl start sshd
#   sudo systemctl enable sshd
#
# === PARTE 2: Crear tu propio servicio ===
#
# PASO 1: Crear el script
# ============================================================

# Crear el script del servicio
cat << 'SCRIPT' > /tmp/mi_servicio_ejemplo.sh
#!/bin/bash
# Este script escribe fecha y hora cada 30 segundos
LOG="/var/log/mi_servicio.log"

while true; do
    echo "$(date '+%Y-%m-%d %H:%M:%S') - Servicio activo - PID: $$" >> "$LOG"
    sleep 30
done
SCRIPT

echo "Script creado en /tmp/mi_servicio_ejemplo.sh"
echo ""
echo "Ahora ejecuta estos pasos manualmente:"
echo ""
echo "# PASO 1: Copiar e instalar el script"
echo "sudo cp /tmp/mi_servicio_ejemplo.sh /opt/mi_servicio.sh"
echo "sudo chmod +x /opt/mi_servicio.sh"
echo ""
echo "# PASO 2: Crear el archivo de unit systemd"
echo "sudo vi /etc/systemd/system/mi_servicio.service"
echo ""
echo "Contenido del archivo:"
echo "---"
cat << 'UNIT'
[Unit]
Description=Mi Servicio de Prueba
After=network.target

[Service]
Type=simple
ExecStart=/opt/mi_servicio.sh
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
UNIT
echo "---"
echo ""
echo "# PASO 3: Recargar systemd, habilitar e iniciar"
echo "sudo systemctl daemon-reload"
echo "sudo systemctl enable mi_servicio.service"
echo "sudo systemctl start mi_servicio.service"
echo ""
echo "# PASO 4: Verificar"
echo "systemctl status mi_servicio.service"
echo "journalctl -u mi_servicio.service -f"
echo "cat /var/log/mi_servicio.log"
echo ""
echo "# PASO 5: Simular falla y verificar reinicio"
echo "kill \$(pgrep -f mi_servicio.sh)"
echo "# Espera 5 segundos y verifica que se reinicio:"
echo "systemctl status mi_servicio.service"
echo ""
echo "# === PARTE 3: Timer systemd ==="
echo ""
echo "# Crear el timer en /etc/systemd/system/mi_servicio.timer"
echo ""
cat << 'TIMER'
[Unit]
Description=Timer para Mi Servicio

[Timer]
OnCalendar=*:0/5
Persistent=true

[Install]
WantedBy=timers.target
TIMER
echo ""
echo "# Modificar el servicio para que NO sea un loop (quitar while)"
echo "# Luego:"
echo "sudo systemctl daemon-reload"
echo "sudo systemctl enable mi_servicio.timer"
echo "sudo systemctl start mi_servicio.timer"
echo "systemctl list-timers"
