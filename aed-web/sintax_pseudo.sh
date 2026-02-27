# Sintaxis de AED
# Herramienta que voy a usar en este caso es grep. Con grep puedo buscar patrones en un archivo de texto.

# Comandos:
# grep -q "texto" "$archivo" -> Buscar si existe un texto en el archivo
# grep -qE "patron" "$archivo" -> Buscar expresiones regulares
# grep -c "texto" "$archivo" -> Contar cuantas veces aparece
# ! grep -> El ! niega: "Si no lo encontro..."
# $? -> El resultado del ultimo comando (0=ok, 1=fallo)
# $((errores + 1)) -> Sumar numeros

#!/bin/bash
archivo="$1" # Esto recibe el archivo como argumento
#$1 es porque puede ser cualquier archivo? o sea de cualquier nivel...

errores=0

# REGLAS

#Regla 1: Debe tener ACCION en la primera linea
if ! head -n 1 "$archivo" | grep -q "^ACCION"; then
    echo "1:ERROR DE SINTAXIS: Falta 'ACCION' al inicio. Siempre al comienzo se define el nombre del archivo con un ACCION nombre del archivo"
# El -q significa silencioso(No imprimer, solo devuelve si encontro o no);
    errores=$((errores + 1))
fi

#Regla 2: Debe tener AMBIENTE
if ! grep -q "AMBIENTE" "$archivo"; then
    echo "0:ERROR DE SINTAXIS: Falta definir 'AMBIENTE'"
    errores=$((errores + 1))
fi

#Regla 3: Debe tener PROCESO

if ! grep -q "PROCESO" "$archivo"; then
    echo "0:ERROR DE SINTAXIS: Falta 'PROCESO'"
    errores=$((errores + 1))
fi

# DENTRO DEL AMBIENTE


# FIN_ACCION
# Sin el -E, grep busca literalmente el texto FIN_ACCION|F_ACCION|FINACCION (con los | incluidos).

if ! grep -qE "FIN_ACCION|F_ACCION|FINACCION" "$archivo"; then
    echo "0:ERROR DE SINTAXIS: Falta cerrar el FIN de la ACCION"
    errores=$((errores + 1))
fi

# Si ya hay errores de estructura basica, salir
if [ $errores -gt 0 ]; then
    exit 1
fi

# SECUENCIAS DE CARACTERES
#
#
# SECUENCIA DE ENTEROS
# SECUENCIA DE SALIDA
# : PARA DEFINIR UNA VARIABLE
# = PARA UNA CONSTANTE
# := ASIGNACION
# ; SIEMPRE TERMINAR
# SI HAY UNA SECUENCIA DE ALGO DEFINIR UNA VENTANA
# VENTANA DE CARACTERES
# VENTANA DE ENTEROS
# ARR() SE USA PARA AVANZAR LAS SECUENCIAS O LAS VENTANAS

# DENTRO DEL PROCESO O ALGORITMO

# AVZ() PARA AVANZAR
# ESCRIBIR() ESTA FUNCION IMPRIME ALGO POR PANTALLA
# LEER() ESTA FUNCION LECTOR DE USUARIO
# Y O OPERADORES LOGICOS
#


# BUCLE MIENTRAS:
#
# MIENTRAS () HACER
#


#
# BUCLE PARA:
#
# PARA ()
#
#

# BUCLE SI


# === VALIDACION DE CONTENIDO ===
# Aca verificamos que el alumno no mande un cascaron vacio
# Usamos grep -n para obtener el numero de linea donde esta cada seccion
# y sed -n para extraer el contenido entre dos lineas

# REGLA 5: AMBIENTE no puede estar vacio
ambiente_linea=$(grep -n "AMBIENTE" "$archivo" | head -1 | cut -d: -f1)
proceso_linea=$(grep -n "PROCESO" "$archivo" | head -1 | cut -d: -f1)
fin_linea=$(grep -nE "FIN_ACCION|F_ACCION|FINACCION" "$archivo" | head -1 | cut -d: -f1)

if [ -n "$ambiente_linea" ] && [ -n "$proceso_linea" ]; then
    # Buscar contenido real entre AMBIENTE y PROCESO (ignorar lineas vacias)
    contenido_ambiente=$(sed -n "$((ambiente_linea + 1)),$((proceso_linea - 1))p" "$archivo" | grep -v "^[[:space:]]*$")
    if [ -z "$contenido_ambiente" ]; then
        echo "${ambiente_linea}:ERROR: La seccion AMBIENTE esta vacia. Defini al menos una variable (ej: precio : REAL)"
        errores=$((errores + 1))
    fi
fi

# REGLA 6: PROCESO no puede estar vacio
if [ -n "$proceso_linea" ] && [ -n "$fin_linea" ]; then
    contenido_proceso=$(sed -n "$((proceso_linea + 1)),$((fin_linea - 1))p" "$archivo" | grep -v "^[[:space:]]*$")
    if [ -z "$contenido_proceso" ]; then
        echo "${proceso_linea}:ERROR: La seccion PROCESO esta vacia. No hiciste nada... Revisa tu codigo"
        errores=$((errores + 1))
    fi
fi

# REGLA 7: Debe tener al menos un ESCRIBIR para mostrar resultado
if ! grep -qi "ESCRIBIR" "$archivo"; then
    echo "0:ERROR: Tu programa no muestra ningun resultado. Usa ESCRIBIR() para mostrar la salida"
    errores=$((errores + 1))
fi

if [ $errores -eq 0 ]; then
    echo "Sintaxis Correcta!"
    exit 0
else
    exit 1
fi
