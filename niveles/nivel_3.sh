#!/bin/bash
# Nivel 3 - Ejercicio 1.1.5.3: Precio PC + Impresora

ENUNCIADO="
  EJERCICIO 1.1.5.3 - Precio total PC + Impresora
  ==================================================

  Se desea comprar una PC y una impresora. Calcular el precio total:
  el cual esta dado por la suma de los precios de costos, los
  porcentajes de ganancia del vendedor y un 21% de IVA.

  Ganancia del vendedor:
    - 12% por la PC
    -  7% por la impresora

  Se leen los costos y se imprime el precio total de ventas.

  Formula:
    precioPC = costoPC * 1.12
    precioImp = costoImpresora * 1.07
    total = (precioPC + precioImp) * 1.21
"

PISTA="Primero sumá la ganancia a cada producto, luego sumá ambos y aplicá el 21% de IVA."

verificar() {
    echo ""
    echo "  PRUEBA DE ESCRITORIO"
    echo "  --------------------"
    echo "  Dados los siguientes valores:"
    echo "    Costo PC       = 1000"
    echo "    Costo Impresora = 500"
    echo "    Ganancia PC     = 12%"
    echo "    Ganancia Imp    = 7%"
    echo "    IVA             = 21%"
    echo ""
    read -p "  Cual es el precio total de venta? > " respuesta

    esperado="2002.55"
    if [ -z "$respuesta" ]; then
        return 1
    fi

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
