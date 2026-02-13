#!/bin/bash
# ============================================================
# FASE 1 - Ejercicio 1.5: Busqueda de archivos
# ============================================================
# OBJETIVO: Dominar find y locate para buscar archivos en el sistema
#
# INSTRUCCIONES:
# 1. Encuentra todos los archivos .conf en /etc
# 2. Encuentra todos los archivos modificados en las ultimas 24 horas en /var/log
# 3. Encuentra todos los archivos mayores a 10MB en /
# 4. Encuentra todos los archivos que pertenecen al usuario root en /tmp
# 5. Encuentra todos los directorios vacios en /home
# 6. Encuentra todos los archivos con permiso SUID en el sistema
#
# COMANDOS CLAVE: find, locate, which, whereis
# ============================================================

echo "=== Ejercicio 1.5 - Busqueda de archivos ==="
echo ""
echo "RESUELVE CADA UNO EN LA TERMINAL:"
echo ""
echo "# 1. Archivos .conf en /etc"
echo "find /etc -name '*.conf' -type f"
echo ""
echo "# 2. Archivos modificados en las ultimas 24 horas"
echo "find /var/log -mtime -1 -type f"
echo ""
echo "# 3. Archivos mayores a 10MB"
echo "find / -size +10M -type f 2>/dev/null"
echo ""
echo "# 4. Archivos de root en /tmp"
echo "find /tmp -user root -type f"
echo ""
echo "# 5. Directorios vacios en /home"
echo "find /home -type d -empty"
echo ""
echo "# 6. Archivos con SUID"
echo "find / -perm -4000 -type f 2>/dev/null"
echo ""
echo "--- EXTRAS PARA PRACTICAR ---"
echo ""
echo "# Encontrar archivos por nombre parcial"
echo "find / -name '*ssh*' -type f 2>/dev/null"
echo ""
echo "# Encontrar y ejecutar comando"
echo "find /tmp -name '*.tmp' -exec rm {} \;"
echo ""
echo "# Usar locate (mas rapido pero necesita updatedb)"
echo "sudo updatedb"
echo "locate httpd.conf"
