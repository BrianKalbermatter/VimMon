#!/bin/bash
# ============================================================
# FASE 3 - Ejercicio 3.6: Tareas programadas (cron, at, timers)
# ============================================================
# OBJETIVO: Programar tareas automaticas
#
# === CRON ===
#
# Formato crontab:
# MIN  HORA  DIA  MES  DIASEM  COMANDO
# 0-59 0-23  1-31 1-12 0-7     /ruta/al/script
#
# Ejemplos:
# */5 * * * *     → cada 5 minutos
# 0 3 * * *       → todos los dias a las 3:00 AM
# 0 0 * * 0       → cada domingo a medianoche
# 30 8 1 * *      → el 1ro de cada mes a las 8:30
#
# 1. Editar crontab de tu usuario
#   crontab -e
#
# 2. Agregar las siguientes tareas:
#
#   # Backup de /etc todos los dias a las 3:00 AM
#   0 3 * * * /usr/bin/tar -czf /tmp/backup_etc_$(date +\%Y\%m\%d).tar.gz /etc 2>/dev/null
#
#   # Limpiar /tmp de archivos mayores a 7 dias cada domingo
#   0 4 * * 0 /usr/bin/find /tmp -type f -mtime +7 -delete 2>/dev/null
#
#   # Registrar uso de disco cada hora
#   0 * * * * /usr/bin/df -h / >> /var/log/disk_usage.log 2>/dev/null
#
# 3. Verificar
#   crontab -l
#
# 4. Ver logs de cron
#   sudo grep CRON /var/log/cron
#   # O en systemd:
#   journalctl -u crond
#
# === AT (tareas de una sola vez) ===
#
# 5. Programar apagado en 2 horas
#   at now + 2 hours
#   > shutdown -h now
#   > <Ctrl+D>
#
# 6. Ver tareas pendientes
#   atq
#
# 7. Cancelar la tarea
#   atrm <numero_de_tarea>
#
# 8. Programar un comando para maniana a las 10:00
#   at 10:00 tomorrow
#   > echo "recordatorio" | mail -s "Test" root
#   > <Ctrl+D>
#
# === BLOQUEAR USUARIOS EN CRON ===
#
# 9. Bloquear a dev1 de usar cron
#   echo "dev1" | sudo tee -a /etc/cron.deny
#
# 10. Verificar (como dev1)
#   sudo -u dev1 crontab -e
#   # Debe dar error: "You (dev1) are not allowed to use this program"
#
# === TIMER SYSTEMD (alternativa moderna a cron) ===
#
# 11. Crear un servicio + timer para backup diario
#
# Archivo: /etc/systemd/system/backup-etc.service
# [Unit]
# Description=Backup de /etc
#
# [Service]
# Type=oneshot
# ExecStart=/usr/bin/tar -czf /tmp/backup_etc.tar.gz /etc
#
# Archivo: /etc/systemd/system/backup-etc.timer
# [Unit]
# Description=Timer para backup de /etc
#
# [Timer]
# OnCalendar=*-*-* 03:00:00
# Persistent=true
#
# [Install]
# WantedBy=timers.target
#
# 12. Habilitar
#   sudo systemctl daemon-reload
#   sudo systemctl enable --now backup-etc.timer
#
# 13. Verificar
#   systemctl list-timers
#   systemctl status backup-etc.timer
#
# IMPORTANTE PARA EL EXAMEN:
# - Saber la sintaxis de cron de memoria
# - Saber usar at para tareas unicas
# - En crontab usar rutas absolutas SIEMPRE
# - Escapar % con \% en crontab
# ============================================================

echo "Este archivo es una guia de referencia para tareas programadas."
echo "Practica creando y verificando crontabs en tu VM."
