#!/learnC/bash
# ============================================================
# FASE 3 - Ejercicio 3.15: Archiving y compresion avanzada
# ============================================================
# OBJETIVO: Dominar tar con extraccion selectiva y distintos
# metodos de compresion. Complementa el 3.6 (cron con tar).
#
# === PARTE 1: Crear backups comprimidos ===
#
# Metodos de compresion:
# -z = gzip  (.tar.gz)  → rapido, compresion media
# -j = bzip2 (.tar.bz2) → mas lento, mejor compresion
# -J = xz    (.tar.xz)  → mas lento aun, mejor compresion
#
# 1. Backup de /etc con gzip
#   tar -czf /backup/etc-backup-$(date +%Y%m%d).tar.gz /etc
#   # -c = create
#   # -z = gzip
#   # -f = file (nombre del archivo)
#
# 2. Backup con bzip2
#   tar -cjf /backup/etc-backup-$(date +%Y%m%d).tar.bz2 /etc
#
# 3. Backup con xz
#   tar -cJf /backup/etc-backup-$(date +%Y%m%d).tar.xz /etc
#
# 4. Comparar tamanos
#   ls -lh /backup/etc-backup*
#   # xz < bzip2 < gzip (en tamano)
#   # gzip > bzip2 > xz (en velocidad)
#
# 5. Crear backup preservando permisos y atributos
#   tar -czpf /backup/etc-full-$(date +%Y%m%d).tar.gz /etc
#   # -p = preserve permissions (importante para backups del sistema)
#
# === PARTE 2: Listar contenido sin extraer ===
#
# 6. Ver contenido de un tar
#   tar -tf /backup/etc-backup-*.tar.gz
#   # -t = list (no extrae nada)
#
# 7. Ver contenido con detalles (como ls -l)
#   tar -tvf /backup/etc-backup-*.tar.gz
#   # -v = verbose
#
# 8. Buscar un archivo especifico dentro del tar
#   tar -tf /backup/etc-backup-*.tar.gz | grep passwd
#   # Muestra la ruta exacta del archivo dentro del tar
#   # Ejemplo de salida: etc/passwd
#
# === PARTE 3: Extraer archivos especificos ===
#
# CLAVE PARA EL EXAMEN: saber extraer UN archivo de un tar grande.
#
# 9. Extraer solo /etc/passwd del backup
#   tar -xzf /backup/etc-backup-*.tar.gz -C /tmp etc/passwd
#   # -x = extract
#   # -C /tmp = extraer en /tmp (change directory)
#   # etc/passwd = ruta del archivo DENTRO del tar (sin / al inicio!)
#
# 10. Verificar
#   ls -l /tmp/etc/passwd
#   cat /tmp/etc/passwd
#   # NOTA: se crea la estructura de directorios: /tmp/etc/passwd
#
# 11. Extraer multiples archivos
#   tar -xzf /backup/etc-backup-*.tar.gz -C /tmp etc/passwd etc/group etc/shadow
#
# 12. Extraer con patron (wildcard)
#   tar -xzf /backup/etc-backup-*.tar.gz -C /tmp --wildcards 'etc/ssh/*'
#   # Extrae todo el directorio etc/ssh/
#
# 13. Extraer SIN la estructura de directorios
#   tar -xzf /backup/etc-backup-*.tar.gz -C /tmp --strip-components=1 etc/passwd
#   # --strip-components=1 quita el primer nivel de directorio
#   # Resultado: /tmp/passwd (en vez de /tmp/etc/passwd)
#
# === PARTE 4: Excluir archivos al crear ===
#
# 14. Backup de /home excluyendo archivos temporales
#   tar -czf /backup/home-backup.tar.gz --exclude='*.tmp' --exclude='*.cache' /home
#
# 15. Excluir un directorio completo
#   tar -czf /backup/home-backup.tar.gz --exclude='/home/*/Downloads' /home
#
# 16. Excluir usando un archivo de patrones
#   cat > /tmp/excluir.txt << 'EOF'
#   *.tmp
#   *.log
#   *.cache
#   .mozilla
#   Downloads
#   EOF
#   tar -czf /backup/home-clean.tar.gz --exclude-from=/tmp/excluir.txt /home
#
# === PARTE 5: Agregar archivos a un tar existente ===
#
# 17. Agregar un archivo a un tar SIN comprimir
#   tar -cf /tmp/datos.tar /etc/hostname
#   tar -rf /tmp/datos.tar /etc/resolv.conf
#   # -r = append
#   # NOTA: -r NO funciona con archivos comprimidos (.tar.gz)
#
# 18. Verificar
#   tar -tf /tmp/datos.tar
#
# === PARTE 6: Cron job para backup automatico ===
#
# 19. Crear script de backup
#   cat > /usr/local/learnC/backup-etc.sh << 'SCRIPT'
#   #!/learnC/bash
#   BACKUP_DIR="/backup"
#   FECHA=$(date +%Y%m%d)
#   mkdir -p "$BACKUP_DIR"
#
#   # Crear backup
#   tar -czpf "${BACKUP_DIR}/etc-${FECHA}.tar.gz" /etc 2>/dev/null
#
#   # Borrar backups de mas de 30 dias
#   find "$BACKUP_DIR" -name "etc-*.tar.gz" -mtime +30 -delete
#
#   # Log
#   echo "$(date): Backup completado - etc-${FECHA}.tar.gz" >> /var/log/backup.log
#   SCRIPT
#   chmod +x /usr/local/learnC/backup-etc.sh
#
# 20. Agregar al crontab (todos los dias a las 2 AM)
#   crontab -e
#   # Agregar:
#   0 2 * * * /usr/local/learnC/backup-etc.sh
#
# 21. Verificar crontab
#   crontab -l
#
# === PARTE 7: Otros formatos ===
#
# 22. Comprimir/descomprimir con gzip (archivo individual)
#   gzip /tmp/archivo.txt         # Crea archivo.txt.gz (borra original)
#   gunzip /tmp/archivo.txt.gz    # Restaura archivo.txt
#   gzip -k /tmp/archivo.txt      # -k = keep original
#
# 23. Comprimir/descomprimir con bzip2
#   bzip2 /tmp/archivo.txt
#   bunzip2 /tmp/archivo.txt.bz2
#
# 24. Comprimir/descomprimir con xz
#   xz /tmp/archivo.txt
#   unxz /tmp/archivo.txt.xz
#
# 25. star (otra herramienta de archiving, a veces mencionada)
#   # No es comun en el examen, pero bueno saberla
#   star -c -f /tmp/backup.star /etc
#   star -t -f /tmp/backup.star
#   star -x -f /tmp/backup.star
#
# === EJERCICIO INTEGRADOR ===
#
# Completar estas tareas:
#
# a) Crear backup de /etc comprimido con gzip en /backup/
#   mkdir -p /backup
#   tar -czpf /backup/etc-$(date +%Y%m%d).tar.gz /etc
#
# b) Listar su contenido y buscar passwd
#   tar -tf /backup/etc-*.tar.gz | grep passwd
#
# c) Extraer SOLO etc/passwd y etc/group a /tmp
#   tar -xzf /backup/etc-*.tar.gz -C /tmp etc/passwd etc/group
#   ls -l /tmp/etc/passwd /tmp/etc/group
#
# d) Crear cron job para backup diario a las 2 AM
#   (crontab -l 2>/dev/null; echo "0 2 * * * tar -czpf /backup/etc-\$(date +\%Y\%m\%d).tar.gz /etc") | crontab -
#   crontab -l
#
# e) Limpiar
#   rm -rf /tmp/etc
#
# === RESUMEN DE COMANDOS ===
#
# CREAR:
# tar -czf archivo.tar.gz RUTA    → crear con gzip
# tar -cjf archivo.tar.bz2 RUTA   → crear con bzip2
# tar -cJf archivo.tar.xz RUTA    → crear con xz
# tar -czpf ... (con -p)          → preservar permisos
#
# LISTAR:
# tar -tf archivo.tar.gz          → listar contenido
# tar -tvf archivo.tar.gz         → listar con detalles
#
# EXTRAER:
# tar -xzf archivo.tar.gz                    → extraer todo
# tar -xzf archivo.tar.gz -C /destino        → extraer en /destino
# tar -xzf archivo.tar.gz -C /tmp etc/passwd → extraer archivo especifico
# tar -xzf ... --wildcards 'etc/ssh/*'       → extraer con patron
# tar -xzf ... --strip-components=N          → quitar N niveles de directorio
#
# IMPORTANTE PARA EL EXAMEN:
# - La ruta dentro del tar NO lleva / al inicio (etc/passwd, no /etc/passwd)
# - Primero listar con -tf para ver la ruta exacta del archivo
# - -C cambia el directorio de destino
# - Siempre usar -p para backups del sistema (preservar permisos)
# - En crontab escapar % con \%
# ============================================================

echo "Este archivo es una guia de referencia para archiving avanzado."
echo "Ejecuta cada comando manualmente en tu VM RHEL 9."
