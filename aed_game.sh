#!/bin/bash
# Con esto le dice al sistema que use bash para ejecutar este script
# ============================================
#  AED Game - Sistema de niveles por ejercicio
# ============================================

# Obtiene la ruta de donde esta el script
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# Con el $0 es el nombre del scritp, dirname saca el directorio, cd ... && pwd lo convierte en ruta absoluta. Esto sirve para que funcione sin importar desde donde lo ejecutes!
# Pregunta: Que significa que sea ruta absoluta?
# Desglose de lo que hace esta variable SCRIPT_DIR="$" asignacion de la variable que va a contener 







NIVELES_DIR="$SCRIPT_DIR/niveles"
SOLUCIONES_DIR="$SCRIPT_DIR/soluciones"
PROGRESO_FILE="$SCRIPT_DIR/.progreso"

# Colores
VERDE='\033[0;32m'
ROJO='\033[0;31m'
AMARILLO='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

# Contar niveles disponibles
TOTAL_NIVELES=$(ls "$NIVELES_DIR"/nivel_*.sh 2>/dev/null | wc -l)

if [ "$TOTAL_NIVELES" -eq 0 ]; then
    echo "Error: No se encontraron niveles en $NIVELES_DIR"
    exit 1
fi

# Leer progreso
if [ -f "$PROGRESO_FILE" ]; then
    NIVEL_ACTUAL=$(cat "$PROGRESO_FILE")
else
    NIVEL_ACTUAL=1
fi

# Validar nivel
if [ "$NIVEL_ACTUAL" -gt "$TOTAL_NIVELES" ]; then
    clear
    echo -e "${VERDE}${BOLD}"
    echo "  =========================================="
    echo "    FELICITACIONES! COMPLETASTE TODOS LOS"
    echo "    NIVELES DE LA GUIA 1.1"
    echo "  =========================================="
    echo -e "${RESET}"
    echo ""
    echo "  Progreso: $TOTAL_NIVELES/$TOTAL_NIVELES niveles completados"
    echo ""
    read -p "  Reiniciar progreso? (s/n) > " reiniciar
    if [ "$reiniciar" = "s" ]; then
        echo "1" > "$PROGRESO_FILE"
        echo "  Progreso reiniciado."
    fi
    exit 0
fi

banner() {
    clear
    echo -e "${CYAN}${BOLD}"
    echo "  ╔══════════════════════════════════════╗"
    echo "  ║     AED - GUIA 1.1 - EJERCICIOS   ║"
    echo "  ╠══════════════════════════════════════╣"
    printf "  ║  NIVEL: %-3s        Progreso: %s/%-4s ║\n" "$NIVEL_ACTUAL" "$((NIVEL_ACTUAL-1))" "$TOTAL_NIVELES"
    echo "  ╚══════════════════════════════════════╝"
    echo -e "${RESET}"
}

# Cargar nivel actual
NIVEL_FILE="$NIVELES_DIR/nivel_${NIVEL_ACTUAL}.sh"
if [ ! -f "$NIVEL_FILE" ]; then
    echo "Error: No se encontro $NIVEL_FILE"
    exit 1
fi

source "$NIVEL_FILE"

# Mostrar banner y enunciado
banner
echo -e "${BOLD}$ENUNCIADO${RESET}"
echo ""

# Barra de progreso
echo -n "  Progreso: ["
for i in $(seq 1 "$TOTAL_NIVELES"); do
    if [ "$i" -lt "$NIVEL_ACTUAL" ]; then
        echo -ne "${VERDE}#${RESET}"
    elif [ "$i" -eq "$NIVEL_ACTUAL" ]; then
        echo -ne "${AMARILLO}>${RESET}"
    else
        echo -n "."
    fi
done
echo "]"
echo ""

# Menu principal
while true; do
    echo "  Opciones:"
    echo "    1) Escribir pseudocodigo (abre editor)"
    echo "    2) Ver mi pseudocodigo actual"
    echo "    3) Verificar (prueba de escritorio)"
    echo "    4) Ver pista"
    echo "    5) Salir"
    echo ""
    read -p "  Elegir opcion > " opcion

    case $opcion in
        1)
            SOL_FILE="$SOLUCIONES_DIR/nivel_${NIVEL_ACTUAL}.txt"
            if [ ! -f "$SOL_FILE" ]; then
                cat > "$SOL_FILE" << 'TEMPLATE'
ACCION ES

AMBIENTE
    // Declarar variables aqui

PROCESO
    // Escribir pseudocodigo aqui

FIN_ACCION
TEMPLATE
            fi
            EDITOR="${EDITOR:-nano}"
            "$EDITOR" "$SOL_FILE"
            echo ""
            echo -e "  ${VERDE}Pseudocodigo guardado en nivel_${NIVEL_ACTUAL}.txt${RESET}"
            echo ""
            ;;
        2)
            SOL_FILE="$SOLUCIONES_DIR/nivel_${NIVEL_ACTUAL}.txt"
            if [ -f "$SOL_FILE" ]; then
                echo ""
                echo -e "${CYAN}  --- Tu pseudocodigo ---${RESET}"
                while IFS= read -r linea; do
                    echo "  $linea"
                done < "$SOL_FILE"
                echo -e "${CYAN}  -----------------------${RESET}"
                echo ""
            else
                echo ""
                echo -e "  ${AMARILLO}Todavia no escribiste pseudocodigo. Usa opcion 1.${RESET}"
                echo ""
            fi
            ;;
        3)
            verificar
            if [ $? -eq 0 ]; then
                echo ""
                echo -e "${VERDE}${BOLD}"
                echo "  ========================================="
                echo "    NIVEL $NIVEL_ACTUAL COMPLETADO!"
                echo "  ========================================="
                echo -e "${RESET}"

                NIVEL_ACTUAL=$((NIVEL_ACTUAL + 1))
                echo "$NIVEL_ACTUAL" > "$PROGRESO_FILE"

                if [ "$NIVEL_ACTUAL" -gt "$TOTAL_NIVELES" ]; then
                    echo ""
                    echo -e "  ${VERDE}Completaste TODOS los niveles!${RESET}"
                    echo ""
                    exit 0
                fi

                echo ""
                read -p "  Continuar al nivel $NIVEL_ACTUAL? (s/n) > " continuar
                if [ "$continuar" = "s" ]; then
                    exec bash "$0"
                else
                    echo "  Progreso guardado. Hasta la proxima!"
                    exit 0
                fi
            else
                echo ""
                echo -e "  ${ROJO}Respuesta incorrecta. Revisá tu prueba de escritorio.${RESET}"
                echo -e "  ${AMARILLO}Tip: Usa opcion 4 para ver una pista.${RESET}"
                echo ""
            fi
            ;;
        4)
            echo ""
            echo -e "  ${AMARILLO}PISTA: $PISTA${RESET}"
            echo ""
            ;;
        5)
            echo "  Progreso guardado en nivel $NIVEL_ACTUAL. Hasta la proxima!"
            exit 0
            ;;
        *)
            echo -e "  ${ROJO}Opcion invalida${RESET}"
            echo ""
            ;;
    esac
done
