#!/bin/bash
# ============================================================
# FASE 1 - Ejercicio 1.4: Vim (obligatorio para RHCSA)
# ============================================================
# OBJETIVO: Dominar vim ya que es el editor disponible en el examen
#
# INSTRUCCIONES:
# 1. Crea un archivo notas.txt con vim
# 2. Escribi 20 lineas de texto cualquiera
# 3. Sin usar el mouse, hace:
#    - Ir a la linea 10                    → :10 o 10G
#    - Borrar las lineas 5 a 8             → :5,8d
#    - Copiar la linea 3 y pegarla al final → :3y luego G p
#    - Buscar "linux" y reemplazar por "LINUX" → :%s/linux/LINUX/g
#    - Guardar y salir                     → :wq
# 4. Abri el archivo de nuevo y deshace todos los cambios (u)
# 5. Practica el modo visual: selecciona un bloque y borralo
#
# REFERENCIA RAPIDA VIM:
# ============================================================

echo "=== Ejercicio 1.4 - Vim ==="
echo ""
echo "REFERENCIA RAPIDA DE VIM:"
echo ""
echo "--- MODOS ---"
echo "i        → modo insercion"
echo "ESC      → volver a modo normal"
echo "v        → modo visual"
echo "V        → modo visual linea"
echo "Ctrl+v   → modo visual bloque"
echo ":        → modo comando"
echo ""
echo "--- MOVIMIENTO ---"
echo "h j k l  → izquierda, abajo, arriba, derecha"
echo "w        → siguiente palabra"
echo "b        → palabra anterior"
echo "0        → inicio de linea"
echo "$        → fin de linea"
echo "gg       → inicio del archivo"
echo "G        → fin del archivo"
echo ":N       → ir a linea N"
echo ""
echo "--- EDICION ---"
echo "dd       → borrar linea"
echo "yy       → copiar linea"
echo "p        → pegar despues"
echo "P        → pegar antes"
echo "u        → deshacer"
echo "Ctrl+r   → rehacer"
echo "x        → borrar caracter"
echo "cw       → cambiar palabra"
echo "cc       → cambiar linea completa"
echo ""
echo "--- BUSQUEDA ---"
echo "/texto   → buscar hacia adelante"
echo "?texto   → buscar hacia atras"
echo "n        → siguiente resultado"
echo "N        → resultado anterior"
echo ":%s/old/new/g  → reemplazar todo"
echo ""
echo "--- ARCHIVOS ---"
echo ":w       → guardar"
echo ":q       → salir"
echo ":wq      → guardar y salir"
echo ":q!      → salir sin guardar"
echo ""
echo "PRACTICA: abri vim y practica cada comando hasta que sea natural"
