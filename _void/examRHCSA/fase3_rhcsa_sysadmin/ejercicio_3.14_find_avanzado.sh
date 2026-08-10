#!/usr/bin/env bash
# ============================================================
# FASE 3 - Ejercicio 3.14: Find avanzado y permisos especiales
# ============================================================
# OBJETIVO: Dominar find con criterios avanzados (SUID, tiempo, tamano)
# Tema clasico del RHCSA - siempre cae alguna pregunta de find.
#
# === PARTE 1: Encontrar archivos SUID ===
#
# SUID (Set User ID): cuando se ejecuta, corre con permisos del DUENO
# del archivo, no del usuario que lo ejecuta.
# Ejemplo: /usr/learnC/passwd tiene SUID → cualquier usuario puede
# cambiar su password aunque /etc/shadow es de root.
#
# 1. Encontrar TODOS los archivos SUID en el sistema
#   find / -perm -4000 -type f 2>/dev/null
#   # -perm -4000 = tiene el bit SUID activado
#   # -type f = solo archivos regulares
#   # 2>/dev/null = ignorar errores de permiso
#
# 2. Guardar la lista en un archivo
#   find / -perm -4000 -type f 2>/dev/null > /root/suid-files.txt
#   cat /root/suid-files.txt
#   wc -l /root/suid-files.txt
#
# 3. Encontrar archivos SGID (Set Group ID)
#   find / -perm -2000 -type f 2>/dev/null > /root/sgid-files.txt
#
# 4. Encontrar archivos que son SUID O SGID
#   find / \( -perm -4000 -o -perm -2000 \) -type f 2>/dev/null
#
# 5. Ver los permisos de los archivos SUID
#   find / -perm -4000 -type f -exec ls -l {} \; 2>/dev/null
#   # La "s" en la posicion del execute del dueno indica SUID:
#   # -rwsr-xr-x → la s reemplaza la x del dueno
#
# === PARTE 2: Buscar por tiempo de modificacion ===
#
# -mtime = modification time (contenido del archivo)
# -atime = access time (ultima lectura)
# -ctime = change time (metadata: permisos, dueno, etc)
#
# 6. Archivos modificados en las ultimas 24 horas en /var/log
#   find /var/log -type f -mtime -1
#   # -mtime -1 = modificado hace MENOS de 1 dia (24h)
#
# 7. Archivos modificados hace MAS de 30 dias
#   find /home -type f -mtime +30
#   # +30 = hace mas de 30 dias
#
# 8. Archivos modificados EXACTAMENTE hace 7 dias
#   find /var -type f -mtime 7
#   # Sin + ni - = exactamente ese numero de dias
#
# 9. Usar -mmin para minutos (mas preciso)
#   find /var/log -type f -mmin -60
#   # Modificados en los ultimos 60 minutos
#
# 10. Archivos modificados entre 2 y 5 dias atras
#   find /var -type f -mtime +2 -mtime -5
#
# 11. Archivos mas nuevos que un archivo de referencia
#   touch -t 202501010000 /tmp/referencia
#   find /etc -type f -newer /tmp/referencia
#   # Archivos modificados despues del 1 enero 2025
#
# === PARTE 3: Buscar por tamano ===
#
# Sufijos: c=bytes, k=kilobytes, M=megabytes, G=gigabytes
#
# 12. Archivos mayores a 100MB en todo el sistema
#   find / -type f -size +100M 2>/dev/null
#
# 13. Con tamano legible
#   find / -type f -size +100M -exec ls -lh {} \; 2>/dev/null
#   # O mas eficiente con +
#   find / -type f -size +100M -exec ls -lh {} + 2>/dev/null
#
# 14. Archivos mayores a 100MB en /home con tamano
#   find /home -type f -size +100M -exec du -h {} \; 2>/dev/null
#
# 15. Archivos entre 10MB y 100MB
#   find / -type f -size +10M -size -100M 2>/dev/null
#
# 16. Archivos vacios
#   find /tmp -type f -empty
#   find /tmp -type d -empty     # directorios vacios
#
# === PARTE 4: Buscar por usuario/grupo ===
#
# 17. Archivos de alice
#   find /home -user alice -type f
#
# 18. Archivos del grupo devops
#   find / -group devops -type f 2>/dev/null
#
# 19. Archivos sin dueno (usuario eliminado)
#   find / -nouser -type f 2>/dev/null
#
# 20. Archivos sin grupo
#   find / -nogroup -type f 2>/dev/null
#
# === PARTE 5: Buscar por permisos ===
#
# 21. Archivos que TODOS pueden escribir (world-writable)
#   find / -perm -o=w -type f 2>/dev/null
#   # O equivalente:
#   find / -perm -002 -type f 2>/dev/null
#
# 22. Directorios world-writable sin sticky bit (inseguro!)
#   find / -perm -o=w ! -perm -1000 -type d 2>/dev/null
#
# 23. Archivos con permisos exactos 777
#   find / -perm 0777 -type f 2>/dev/null
#
# 24. Archivos ejecutables
#   find /usr/local/learnC -type f -executable
#
# === PARTE 6: Acciones con find ===
#
# 25. Copiar archivos encontrados
#   find /var/log -name "*.log" -mtime -1 -exec cp {} /tmp/logs_recientes/ \;
#
# 26. Cambiar permisos de archivos encontrados
#   find /shared -type f -exec chmod 644 {} +
#   find /shared -type d -exec chmod 755 {} +
#
# 27. Borrar archivos viejos (CUIDADO)
#   find /tmp -type f -mtime +30 -delete
#   # -delete es mas eficiente que -exec rm
#
# 28. Contar archivos por tipo
#   find /etc -type f -name "*.conf" | wc -l
#
# === PARTE 7: Ejercicio integrador ===
#
# Completar estas tareas y guardar resultados:
#
# a) Archivos SUID → /root/audit/suid.txt
#   mkdir -p /root/audit
#   find / -perm -4000 -type f 2>/dev/null > /root/audit/suid.txt
#
# b) Archivos modificados hoy en /etc → /root/audit/etc-hoy.txt
#   find /etc -type f -mtime -1 2>/dev/null > /root/audit/etc-hoy.txt
#
# c) Archivos > 50MB → /root/audit/grandes.txt
#   find / -type f -size +50M -exec ls -lh {} + 2>/dev/null > /root/audit/grandes.txt
#
# d) Archivos sin dueno → /root/audit/sin-dueno.txt
#   find / -nouser -o -nogroup 2>/dev/null > /root/audit/sin-dueno.txt
#
# e) Archivos world-writable → /root/audit/world-writable.txt
#   find / -perm -o=w -type f 2>/dev/null > /root/audit/world-writable.txt
#
# === DIFERENCIA ENTRE -exec \; y -exec + ===
#
# -exec comando {} \;  → ejecuta el comando UNA VEZ por cada archivo
#   find /tmp -name "*.txt" -exec rm {} \;
#   # Equivale a: rm file1.txt; rm file2.txt; rm file3.txt
#
# -exec comando {} +   → ejecuta el comando UNA VEZ con todos los archivos
#   find /tmp -name "*.txt" -exec rm {} +
#   # Equivale a: rm file1.txt file2.txt file3.txt
#   # MAS EFICIENTE!
#
# === RESUMEN DE COMANDOS ===
#
# find PATH -perm -4000           → archivos SUID
# find PATH -perm -2000           → archivos SGID
# find PATH -mtime -1             → modificados < 1 dia
# find PATH -mtime +30            → modificados > 30 dias
# find PATH -mmin -60             → modificados < 60 minutos
# find PATH -size +100M           → mayores a 100MB
# find PATH -size -1k             → menores a 1KB
# find PATH -user USER            → archivos del usuario
# find PATH -nouser               → archivos sin dueno
# find PATH -perm -o=w            → world-writable
# find PATH -empty                → vacios
# find PATH -newer REF            → mas nuevos que REF
# find PATH ... -exec CMD {} +    → ejecutar comando
# find PATH ... -delete           → borrar encontrados
#
# IMPORTANTE PARA EL EXAMEN:
# - Siempre redirigir stderr: 2>/dev/null
# - -perm -4000 para SUID (el - significa "al menos estos bits")
# - -mtime usa DIAS, -mmin usa MINUTOS
# - + es "mas que", - es "menos que", sin signo es "exactamente"
# - Preferir -exec {} + sobre -exec {} \; (mas eficiente)
# ============================================================

echo "Este archivo es una guia de referencia para find avanzado."
echo "Ejecuta cada comando manualmente en tu VM RHEL 9."
