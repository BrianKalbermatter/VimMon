#!/bin/bash

# raw -> no espera Enter para enviar input, cada tecla llega inmediato.
# -echo -> no muestra lo que escibis en pantalla (controlo lo que se va a mostrar)

# stty sane -> Si tu script crashea sin ejecutar esto, tu terminal queda roto (escribis stty sane a ciegas para arreglarlo).

# Pregunta: Cada byte es una tecla ?
# read -rsn1 char -> Lee exactamente 1 byte: Lo que hace esto es leer un caracter del teclado
# -r -> No interpreta backslashes(\).
# -s -> Silencioso (no echo)
# -n1 -> lee 1 caracter
#
# trap -> Para limpiar al salir, pase lo que pase
echo "⠀⠀⠀⠀⠀⠀⠀⠀⠀       ⣀⠀⠀⠀⠀⠀⠀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡠⢒⣩⣥⠡⠀⠀⠀⠀⠌⣬⣍⡒⢄⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡴⠫⣶⣿⣿⣿⣧⡑⢄⡠⢊⣼⣿⣿⣿⣦⠝⢆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡜⡅⣿⣦⡙⢿⣿⣿⣿⣦⣴⣿⣿⣿⡿⢋⣴⣿⢨⢣⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⡜⣼⡇⣿⣿⣿⣦⡹⣿⣿⣿⣿⣿⣿⢋⣴⣿⣿⣿⢸⣧⢣⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⣿⠗⠛⠻⠿⣿⣷⡘⡿⠟⠻⢿⢃⣾⣿⠿⠟⠛⠺⣿⣧⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠸⠟⠀⣀⢀⣄⣀⠈⠙⡗⣰⠿⠿⣆⢺⠋⠁⣀⣤⡀⣀⠈⠻⠇⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⢀⢦⣍⡛⠶⣤⠤⠜⣀⣤⠀⣶⣶⠀⣤⣐⠣⢤⣤⠶⢛⣩⡶⡀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠈⢧⡙⢿⣷⡆⣶⣿⡿⢋⢸⣿⣿⡆⡙⢿⣿⣶⢰⣾⡿⢋⡼⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢙⠦⣍⠰⣿⡟⣡⣿⢸⣿⣿⡇⣿⡌⢻⣿⠆⣩⠶⡋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠐⢌⢷⢈⢳⣌⠁⣤⡌⢸⣿⣿⡇⢡⣤⠈⣡⡞⡁⡾⡡⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⡎⣧⡹⢰⣿⣷⣈⠛⠛⣡⣾⣿⡆⢏⣼⢸⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡇⣿⣇⢸⣿⣿⣿⣿⣿⣿⣿⣿⡇⣸⣿⢸⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠃⣿⣿⣄⠙⢉⣩⣉⣉⣍⡉⠋⣨⣿⣿⠘⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠐⣄⠻⣿⣿⣷⡆⢀⣀⣀⡀⢰⣾⣿⣿⠟⣠⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠳⣌⠻⢋⡤⠉⠉⠉⠁⣤⡙⠟⣡⠞⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⠿⣿⡇⠀⠀⠀⠀⢸⣿⠟⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠁⠀⠀⠀⠀⠈⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
                 |BIM.|
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"
cleanup() {
    echo "Volviendo a su modo Normal"
    stty sane # Restaura al terminal a su modo normal
}
trap cleanup EXIT
stty raw -echo # Esto pone en modo raw la terminal!

while true; do
    # read -rsn1 char
    # Si es 'q', salir
    # si no, mostrar el codigo del caracter!
    # printf '%d \r\n' "'$char"
    if [[ "$char" == "[A" ]]; then
        #Lo que hace el -rsn2 lee solo dos bytes nomas y sino es -rsn1, da igual
        #Lo que hace el -t es que hace un timeout como un sleep
        read -rsn2 -t 0.05 rest
        printf 'Flecha Arriba\r\n'
    elif [[ "$rest" == "[B" ]]; then
        read -rsn2 -t 0.05 rest
        printf 'Flecha Abajo\r\n'
    elif [[ "$rest" == "[C" ]]; then
        printf 'Flecha Derecha\r\n'
    elif [[ "$rest" == "[D" ]]; then
        printf 'Flecha Izquierda\r\n'
    elif [[ "$char" == "q" ]]; then
        read -rsn1 -t 0.05 rest
        break
    else 
        printf 'ESC\r\n'
    fi
done







: <<'END_COMMENT'
  Tu primera tarea:

  Creá el archivo bim.sh con esta estructura (escribí vos el código):

  #!/bin/bash

  # 1. Una función "cleanup" que restaure el terminal con stty sane

  # 2. Un trap para que cleanup se ejecute siempre al salir:
  #    trap cleanup EXIT

  # 3. Poner el terminal en modo raw: stty raw -echo

  # 4. Un loop infinito (while true) que:
  #    a. Lea un carácter con read -rsn1
  #    b. Si es 'q', hacer break (salir)
  #    c. Si no, imprimir el código ASCII del carácter
  #       Pista: printf '%d' "'$char"  te da el código numérico



  Sobre las flechas (clave):

  Las teclas especiales como flechas envían 3 bytes en secuencia:
  - \e (ESC, código 27) + [ + una letra (A=arriba, B=abajo, C=derecha, D=izquierda)

  Entonces después de leer un byte, si es ESC (código 27), tenés que leer 2 bytes más con read -rsn2
  -t 0.01 (el timeout es para distinguir un ESC solo de una secuencia de escape).

  Para probar:

  chmod +x bim.sh
  ./bim.sh

  Presioná teclas y fijate qué códigos te muestra. Probá letras normales, flechas, Enter, Backspace,
  ESC.

  ---
  Preguntas para que pienses:

  1. Qué pasa si no ponés el trap? (probalo)
  2. Qué código devuelve Enter? Y Backspace? (hay una sorpresa con Backspace: puede ser 127 o 8)
  3. Cómo distinguirías ESC solo (para cambiar de modo) de ESC como inicio de una secuencia de flecha?


cleanup(){

}
END_COMMENT








