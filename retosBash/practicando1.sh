#!/usr/bin/env bash

# $1 = primer numero
# $2 = operador
# $3 = segundo numero

# $digo que estoy llamando a la variable 1
case "$2" in
    +) echo $(( $1 + $3 )) ;;
    /) echo $(( $1 / $3 )) ;;
    # * -> significa comodin y los atrapa primero. Tengo que escaparlo con \* o '*'
    \*) echo $(( $1 * $3 )) ;;
    -) echo $(( $1 - $3 )) ;;
    %) echo $(( $1 % $3 )) ;;
esac
