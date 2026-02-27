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

==> Presiona cualquier Tecla!
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"
read -rsn1


cleanup() {
    echo "Volviendo a su modo Normal"
    stty sane # Restaura al terminal a su modo normal
}


#VARIABLES
cursor_row=0 #en que linea del buffer estas (empieza en 0)
cursor_col=0 # en que columna estas (empieza en 0)
modo="NORMAL" # Es para iniciar en Normal!



# Esta funcion lo que hace es lee un archivo y mete cada linea en una posicion de un array
# $1 es el primer argumento que le pasas al script en el que estas analizando el archivo... como por ejemplo /bin.sh archivoPrueba.txt
# -t quita el \n salto de linea del final de cada linea.
# buffer es el array donde se guardan
#   Si tu archivo tiene:
#  hola
#  mundo
#  chau
# Después de mapfile, el array queda:
#  buffer[0] = "hola"
#  buffer[1] = "mundo"
#  buffer[2] = "chau"
mapfile -t buffer < "$1"


FILAS=$(tput lines)
COLS=$(tput cols)
trap cleanup EXIT
stty raw -echo # Esto pone en modo raw la terminal!
printf '\e[2J\e[H'    ← limpia una sola vez antes del loop



# - \e[2J -> Borra todo
# - \e[H -> Cursor a posicion 1,1



mensaje=""

CLEAN_SCREEN='\e[H'
BG='\e[48;2;40;40;40m'
FG='\e[38;2;235;219;178m'
AMARILLO='\e[38;2;250;189;47m'
BARRA_BG='\e[48;2;80;73;69m'
RESET='\e[0m'


# Funcion de modoNormal
# Aca va a ir la logica de movimiento del cursor, i, q.
modeNormal() {
    #LEER TECLA
    read -rsn1 char

    # read -rsn2 rest  
    # Si es 'q', salir
    # si no, mostrar el codigo del caracter!
    # printf '%d \r\n' "'$char"
##############################################
    if [[ "$char" == $'\e' ]];
    then
        read -rsn2 -t 0.05 rest
##############################################
        # Flecha Arriba
        if [[ "$rest" == "[A" ]]; 
        then
        #Lo que hace el -rsn2 lee solo dos bytes nomas y sino es -rsn1, da igual
        #Lo que hace el -t es que hace un timeout como un sleep
        cursor_row=$((cursor_row - 1))
        if ((cursor_row < 0)); 
        then 
            cursor_row=0; 
        fi 
##############################################
            # Flecha Abajo
            elif [[ "$rest" == "[B" ]]; 
            then
                cursor_row=$((cursor_row + 1))
                if ((cursor_row > ${#buffer[@]} - 1)); 
                then
                    cursor_row=$((${#buffer[@]} - 1)); 
                fi
##############################################
            # Flecha Derecha
            elif [[ "$rest" == "[C" ]];
            then
                cursor_col=$((cursor_col + 1))
                if ((cursor_col > ${#buffer[$cursor_row]})); 
                then 
                    cursor_col=${#buffer[$cursor_row]}; 
                fi
##############################################
            # Flecha Izquierda
            elif [[ "$rest" == "[D" ]]; 
            then
                cursor_col=$((cursor_col - 1))
                if ((cursor_col < 0)); 
                then 
                    cursor_col=0; 
                fi
            fi
##############################################
    elif [[ "$char" == "i" ]]; 
    then
         modo="INSERT"
    elif [[ "$char" == "q" ]];
    then
        quit=1
    fi
}


















# Funcion de modoInsert
# Aca va ESC para volver, y despues escribir texto
modeInsert() {
    read -rsn1 char
    echo "TECLA: char=[$char] char=[$(printf '%d' "'$char")]" >> /tmp/bim_debug.log
    if [[ "$char" == $'\e' ]]; then
        modo="NORMAL"
    # backspace:
    elif [[ "$char" == $'\x7f' ]]; then
        if (( cursor_col > 0 )); then
            buffer[$cursor_row]="${buffer[$cursor_row]:0:$((cursor_col-1))}${buffer[$cursor_row]:$cursor_col}"
                cursor_col=$((cursor_col - 1))
        fi
    # enter:    
    elif [[ "$char" == $'\r' ]]; then
        #El buffer como un array de líneas:
        #
        #  buffer[0] = "hola mundo"
        #  buffer[1] = "chau"
        #  buffer[2] = "adios"
        #
        #  Cuando apretás Enter en el medio de "hola mundo" con el cursor en la
        #   posición 4:
        #
        #  "hola| mundo"
        #        ↑ cursor_col=4
        #
        #  Necesitás convertir una línea en dos:
        #  buffer[0] = "hola"
        #  buffer[1] = " mundo"   ← línea nueva insertada
        #  buffer[2] = "chau"     ← se corrió para abajo
        #  buffer[3] = "adios"    ← se corrió para abajo
        # Teniendo en cuenta esto: 
        #       El buffer es un array definido que se carga con mapfile, por ejemplo selecciono un archivo.txt cualquiera y lo que hace eso es que cada linea dentro de ese archivo se guarde.

        local antes="${buffer[$cursor_row]:0:$cursor_col}"
        # Agarra todo lo que esta antes del cursor. En el ejemplo: "hola". Lo guarda buffer

        local despues="${buffer[$cursor_row]:$cursor_col}"
        # Agarra todo lo que esta despues del cursor. En el ejemplo: "mundo". Lo guarda en el buffer

        buffer[$cursor_row]="$antes"
        # La linea actual queda solo con la parte de antes: "hola"

        buffer=(${buffer[@]:0:$((cursor_row+1))} "$despues" ${buffer[@]:$((cursor_row+1))})
        # Imaginá que cursor_row=0 y el buffer tiene:

        # buffer[0] = "hola"    ← ya modificado en el paso anterior
        # buffer[1] = "chau"
        # buffer[2] = "adios"

        # La línea arma un nuevo array en 3 pedazos:

        # ${buffer[@]:0:$((cursor_row+1))}   →  buffer[0]="hola"          (desde 0, toma 1 elemento)
        # "$despues"                         →  " mundo"                   (la línea nueva)
        # ${buffer[@]:$((cursor_row+1))}     →  buffer[1]="chau", buffer[2]="adios"  (el resto)

        # Resultado:
        # buffer[0] = "hola"
        # buffer[1] = " mundo"   ← insertada
        # buffer[2] = "chau"     ← se corrió
        # buffer[3] = "adios"    ← se corrió

        cursor_row=$((cursor_row + 1)) # El cursor baja a la linea nueva.
        cursor_col=0 # El cursor va al principio de esa linea
    else

        # El cursor está en la posición 7 (cursor_col=7).
        # Apretás la tecla "X":
        # antes  = "hola a"          → ${buffer[$cursor_row]:0:7}
        # char   = "X"
        # despues = " como estas?"   → ${buffer[$cursor_row]:7}
        # Resultado:
        # "hola aX como estas?"
        # Y el cursor se mueve a cursor_col=8, justo después de la X.
        # ---
        # Eso es todo lo que hace ese else. Simple pero elegante.
        buffer[$cursor_row]="${buffer[$cursor_row]:0:$cursor_col}${char}${buffer[$cursor_row]:$cursor_col}"
        cursor_col=$((cursor_col + 1))
        
    fi
}
































# Funcion LOOP El Inicio
loopStart() {
    #---------------LOOP----------------
    while true; do
        printf "${BG}"    # Fondo gruvbox
        printf "${FG}" # Texto gruvbox
        
        # Este printf() limpia la pantalla y pone el cursor arriba
        printf "${CLEAN_SCREEN}"
        # - \e[2J -> Borra todo
        # - \e[H -> Cursor a posicion 1,1
        #Empieza el SHOW!
        #Imprime un mensaje
        #printf "${AMARILLO}==> %s${FG}" "$mensaje"
        # En un for en bash necesita el formato completo con (()) y las tres partes: inicio, condicion, incremento
        # Se empieza en la linea 0 del array(cada linea de un archivo de codigo o de texto o de lo que sea es un 
        # array, una lista de cosas, caracteres, numeros, etc.)
        # i<${#buffer[@]} : Mientras i sea menor que la cantidad de lineas
        
        for ((i=0; i<${#buffer[@]}; i++)); do
            #$((i+1)) te da el número de línea (porque i arranca en 0 pero las líneas se muestran desde 1).
            printf "\e[$((i+1));1H %3d %s" "$((i+1))" "${buffer[$i]}"
        done
        
        #Para un string y lo imprima '' y el "" para comando, ejemplo:
        # El %-${COLS}s le dice a printf: "imprimi este string y rellenalo con espacios hasta ocupar $COLS caracteres". 
        # Asi la barra invertida cubre toda la linea.
        printf "\e[${FILAS};1H${BARRA_BG}${FG}%-${COLS}s${RESET}"  " | $modo | " # ir a la ultima fila
        # Esto fuerza al cursor a ser visible. Algunas terminales ocultan cuando dibujas mucho...
        printf '\e[?25h'
        printf "\e[$((cursor_row+1));$((cursor_col+5))H"
        # Porque +5? Porque las primeras columnas las ocupan los numeros de linea (1). Ajusta ese numero segun cuanto 
        # espacio le diste a los numeros.
        #############################################################################################################
        if [[ "$modo" == NORMAL ]];
        then
            modeNormal
        elif [[ "$modo" == "INSERT" ]];
        then
            modeInsert
        fi
        if [[ "$quit" == 1 ]];
        then
            break
        fi
    done
}

# Solo llama al loopStart porque es donde esta toda la logica y luego llama a las diferentes funciones dentro
loopStart

