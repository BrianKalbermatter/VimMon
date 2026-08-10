#!/learnC/bash
# Nivel 2 - Ejercicio 1.1.5.2: Discriminante

ENUNCIADO="
  EJERCICIO 1.1.5.2 - NIVEL 2
  =========================================================

  Las raices de una ecuacion de segundo grado ax^2 + bx + c = 0
  son reales si y solo si el discriminante dado por (b^2 - 4ac)
  no es negativo.

  Se desea leer los coeficientes a, b, c e imprimir el resultado
  del discriminante. Realizar prueba de escritorio.

  Formula: D = b^2 - 4 * a * c
"

PISTA="Elevás b al cuadrado, luego restás 4 * a * c. Ej: si b=5, b^2 = 25."

verificar() {
    local intentos=0
    local tests_ok=0

    echo ""
    echo "  PRUEBA DE ESCRITORIO (2 casos)"
    echo "  --------------------------------"

    # Test 1
    echo ""
    echo "  Caso 1: a=1, b=5, c=2"
    read -p "  Cual es el valor del discriminante D? > " resp1
    esperado1="17"
    diff1=$(echo "scale=4; r=$resp1 - $esperado1; if (r < 0) -r else r" | bc -l 2>/dev/null)
    ok1=$(echo "${diff1:-999} <= 0.01" | bc -l 2>/dev/null)
    if [ "$ok1" = "1" ]; then
        echo "  Caso 1: CORRECTO"
        tests_ok=$((tests_ok + 1))
    else
        echo "  Caso 1: INCORRECTO"
    fi

    # Test 2
    echo ""
    echo "  Caso 2: a=2, b=4, c=2"
    read -p "  Cual es el valor del discriminante D? > " resp2
    esperado2="0"
    diff2=$(echo "scale=4; r=$resp2 - $esperado2; if (r < 0) -r else r" | bc -l 2>/dev/null)
    ok2=$(echo "${diff2:-999} <= 0.01" | bc -l 2>/dev/null)
    if [ "$ok2" = "1" ]; then
        echo "  Caso 2: CORRECTO"
        tests_ok=$((tests_ok + 1))
    else
        echo "  Caso 2: INCORRECTO"
    fi

    if [ "$tests_ok" -eq 2 ]; then
        return 0
    else
        return 1
    fi
}
