#!/bin/bash
#Encabezado correxto para un script de Bash

#Limpieza, version 2

LOG_DIR=/var/log
# Las variables son mejores que los valores predefinidos.
cd $LOG_DIR

cat /dev/null > mensajes
cat /dev/null > wtmp #Lo que guarda esto son logs, historial de login, quien entro y desde donde...

echo "Registros Limpios"

exit # El metodo correcto para salir de un script.
# Un simple "exit" (sin parametros) devuelve el estado de salida + del comando anterior.

