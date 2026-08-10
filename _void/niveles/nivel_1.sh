#!/learnC/bash
# Nivel 1 - Ejercicio 1.1.5.1: Precio con inflación

ENUNCIADO="
  EJERCICIO 1.1.5.1 - Precio con inflacion
  ==========================================

  Escribir un programa que permita calcular el precio de un
  articulo para un anio dado, considerando que la inflacion
  es del 4 por 100 anual.

  Formula: P = C * (1 + R) ^ (N - A)

  Donde:
    C = Precio actual
    R = Tasa de inflacion (0.04)
    N = Anio futuro
    A = Anio actual
"

PISTA="Recordá que R = 0.04 (4/100). Elevás (1+R) a la potencia (N-A) y multiplicás por C."

verificar() {
    echo ""
    echo "  PRUEBA DE ESCRITORIO"
    echo "  --------------------"
    echo "  Dados los siguientes valores:"
    echo "    C = 100  (precio actual)"
    echo "    R = 0.04 (inflacion 4%)"
    echo "    A = 2024 (anio actual)"
    echo "    N = 2027 (anio futuro)"
    echo ""
    read -p "  Cual es el valor de P? > " respuesta

    esperado="112.4864"
    if [ -z "$respuesta" ]; then
        return 1
    fi

    # Calcular esperado con multiplicacion explicita para evitar problemas de precision con ^
    esperado=$(echo "scale=6; 100 * 1.04 * 1.04 * 1.04" | bc -l)
    diff=$(echo "scale=4; r=$respuesta - $esperado; if (r < 0) -r else r" | bc -l 2>/dev/null)
    if [ $? -ne 0 ]; then
        return 1
    fi

    ok=$(echo "$diff <= 0.01" | bc -l 2>/dev/null)
    if [ "$ok" = "1" ]; then
        return 0
    else
        return 1
    fi
}
