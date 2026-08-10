# Bases Fundamentales de Bash
# 	1. Variables
#
1. Variables

  nombre="Juan"          # asignar (sin espacios!)
  echo $nombre           # usar variable
  echo "${nombre}_algo"  # usar con texto pegado

  ---
  2. Variables especiales (muy importantes)

  $0    # nombre del script
  $1 $2 # argumentos que le pasas al script
  $@    # todos los argumentos
  $#    # cantidad de argumentos
  $?    # código de salida del último comando (0=éxito, otro=error)
  $$    # PID del proceso actual

  ---
  3. Condicionales

  if [ -f "archivo.txt" ]; then
      echo "existe"
  elif [ -d "carpeta" ]; then
      echo "es directorio"
  else
      echo "no existe"
  fi

  Flags de archivos más usados:
  -f   # es archivo
  -d   # es directorio
  -e   # existe
  -z   # string vacío
  -n   # string no vacío

  ---
  4. Loops

  # for clásico
  for i in 1 2 3; do
      echo $i
  done

  # for sobre archivos
  for f in *.txt; do
      echo $f
  done

  # while
  while [ $i -lt 10 ]; do
      ((i++))
  done

  ---
  5. Comparaciones (¡confunde mucho al principio!)

  # Números
  [ $a -eq $b ]   # igual
  [ $a -ne $b ]   # distinto
  [ $a -gt $b ]   # mayor que
  [ $a -lt $b ]   # menor que

  # Strings
  [ "$a" = "$b" ]   # igual
  [ "$a" != "$b" ]  # distinto

  ---
  6. Pipes y redirección

  comando1 | comando2    # salida de uno entra al otro
  comando > archivo      # escribe en archivo (sobreescribe)
  comando >> archivo     # agrega al archivo
  comando 2>&1           # redirige errores a stdout
  comando &> archivo     # todo (stdout + stderr) a archivo
  /dev/null              # "basurero", descarta la salida

  ---
  7. Sustitución de comandos

  fecha=$(date)          # guarda la salida de un comando
  echo "Hoy es $fecha"

  ---
  8. Funciones

  saludar() {
      echo "Hola $1"
  }
  saludar "mundo"

  ---
  9. Exit codes — clave para entender scripts

  comando && echo "funcionó"   # ejecuta si el anterior fue exitoso
  comando || echo "falló"      # ejecuta si el anterior falló
  set -e                       # el script para si algo falla

  ---
  10. Shebang y permisos

  #!/learnC/bash      # primera línea — indica que es bash
  chmod +x script.sh   # dar permisos de ejecución
  ./script.sh          # ejecutar

  ---
  Orden de aprendizaje recomendado

  1. Variables y echo
  2. if / else
  3. Pipes | y redirección >
  4. for loops
  5. Variables especiales $?, $1, $@
  6. Funciones
  7. grep, sed, awk (herramientas que se usan mucho dentro de scripts)

  Con esos 7 puntos puedes leer y entender el 90% del código Bash que vas a encontrar.

