#!/bin/bash
# ============================================================
# FASE 1 - Ejercicio 1.1: Estructura de directorios
# ============================================================
# OBJETIVO: Practicar creacion y manipulacion de archivos/directorios
#
# INSTRUCCIONES:
# Crea la siguiente estructura solo con la terminal:
#
# /home/tu_usuario/proyecto/
# ├── src/
# │   ├── main.c
# │   ├── utils.c
# │   └── utils.h
# ├── bin/
# ├── docs/
# │   └── README
# ├── tests/
# │   ├── test1.sh
# │   └── test2.sh
# └── Makefile
#
# TAREAS:
# 1. Crea toda la estructura con mkdir y touch
# 2. Copia todo el directorio proyecto/ a /tmp/backup_proyecto/
# 3. Crea un link simbolico de /tmp/backup_proyecto a ~/backup
# 4. Verifica que el link funciona con ls -la
# 5. Borra tests/test2.sh y verifica que ya no existe
#
# COMANDOS CLAVE: mkdir -p, touch, cp -r, ln -s, rm, ls -la
# ============================================================

echo "=== Ejercicio 1.1 - Estructura de directorios ==="
echo ""
echo "Resuelve este ejercicio manualmente en la terminal."
echo "No ejecutes este script, usalo como guia."
echo ""
echo "PASOS:"
echo "1. mkdir -p ~/proyecto/{src,bin,docs,tests}"
echo "2. touch ~/proyecto/src/{main.c,utils.c,utils.h}"
echo "3. touch ~/proyecto/docs/README"
echo "4. touch ~/proyecto/tests/{test1.sh,test2.sh}"
echo "5. touch ~/proyecto/Makefile"
echo "6. cp -r ~/proyecto /tmp/backup_proyecto"
echo "7. ln -s /tmp/backup_proyecto ~/backup"
echo "8. ls -la ~/backup"
echo "9. rm ~/proyecto/tests/test2.sh"
echo "10. ls ~/proyecto/tests/"
echo ""
echo "VERIFICA: tree ~/proyecto (instala tree si no lo tenes)"
