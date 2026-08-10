# Sintaxis de AED
# Herramienta que voy a usar en este caso es grep. Con grep puedo buscar patrones en un archivo de texto.

# Comandos:
# grep -q "texto" "$archivo" -> Buscar si existe un texto en el archivo
# grep -qE "patron" "$archivo" -> Buscar expresiones regulares
# grep -c "texto" "$archivo" -> Contar cuantas veces aparece
# ! grep -> El ! niega: "Si no lo encontro..."
# $? -> El resultado del ultimo comando (0=ok, 1=fallo)
# $((errores + 1)) -> Sumar numeros

#!/usr/bin/env bash
archivo="$1" # Esto recibe el archivo como argumento
#$1 es porque puede ser cualquier archivo? o sea de cualquier nivel...

errores=0

# REGLAS

#Regla 1: Debe tener ACCION
if ! grep -q "^ACCION" "$archivo"; then
    echo "ERROR DE SINTAXIS: Falta 'ACCION' al inicio. Siempre al comienzo se define el nombre del archivo con un ACCION nombre del archivo"
# El -q significa silencioso(No imprimer, solo devuelve si encontro o no);
    errores=$((errores + 1))
fi

#Regla 2: Debe tener AMBIENTE
if ! grep -q "AMBIENTE" "$archivo"; then
    echo "ERROR DE SINTAXIS: Falta definir 'AMBIENTE'"
    errores=$((errores + 1))
fi

#Regla 3: Debe tener PROCESO

if ! grep -q "PROCESO" "$archivo"; then
    echo "ERROR DE SINTAXIS: Falta 'PROCESO'"
    errores=$((errores + 1))
fi

# DENTRO DEL AMBIENTE


# FIN_ACCION
# Sin el -E, grep busca literalmente el texto FIN_ACCION|F_ACCION|FINACCION (con los | incluidos).

if ! grep -qE "FIN_ACCION|F_ACCION|FINACCION" "$archivo"; then
    echo "ERROR DE SINTAXIS: Falta cerrar el FIN de la ACCION"
    errores=$((errores + 1))
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










































if [ $errores -eq 0 ]; then
    echo "Sintaxis Correcta!"
    exit 0
else
    echo "Se encontraron $errores errores"
    exit 1
fi
