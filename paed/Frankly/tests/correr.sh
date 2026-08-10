#!/usr/bin/env bash
# Corre cada programa .paed de esta carpeta y compara su salida contra el
# .esperado de al lado.
#
# Un test que hay que mirar no es un test: antes de esto, probar el lenguaje
# era abrir la ventana SDL y confiar en la vista. Aca la salida se compara
# caracter por caracter y el exit code dice si paso.
#
# Agregar un test = dejar el .paed y su .esperado en esta carpeta. Nada mas:
# no hay lista que mantener a mano.
#
#   bash paed/Frankly/tests/correr.sh          corre todo
#   ACTUALIZAR=1 bash .../correr.sh            regraba los .esperado
#
# ACTUALIZAR se usa cuando el cambio de salida es a proposito. Mirar el diff
# de los .esperado en el commit ANTES de darlo por bueno: si se regraban sin
# leerlos, el test deja de proteger nada.

set -uo pipefail

# La raiz del repo: dos niveles arriba de paed/Frankly/tests
raiz=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)
cd "$raiz" || exit 1

runner=build/paedrun
if [ ! -x "$runner" ]; then
    echo "falta $runner — corre 'make paedrun' primero" >&2
    exit 1
fi

tests_dir=paed/Frankly/tests
pasaron=0
fallaron=0
fallidos=()

for prog in "$tests_dir"/*.paed; do
    [ -e "$prog" ] || continue
    esperado="${prog%.paed}.esperado"
    nombre=$(basename "$prog")

    # stderr se junta con stdout: los mensajes de error son parte de lo que se
    # verifica. Un error con el texto equivocado es un test fallado.
    real=$("$runner" "$prog" 2>&1)

    if [ "${ACTUALIZAR:-0}" = "1" ]; then
        printf '%s\n' "$real" > "$esperado"
        echo "  grabado  $nombre"
        continue
    fi

    if [ ! -f "$esperado" ]; then
        echo "  SIN ESPERADO  $nombre (crealo con ACTUALIZAR=1)"
        fallaron=$((fallaron + 1))
        fallidos+=("$nombre")
        continue
    fi

    if [ "$real" = "$(cat "$esperado")" ]; then
        echo "  ok       $nombre"
        pasaron=$((pasaron + 1))
    else
        echo "  FALLA    $nombre"
        diff -u "$esperado" <(printf '%s\n' "$real") | sed 's/^/           /'
        fallaron=$((fallaron + 1))
        fallidos+=("$nombre")
    fi
done

[ "${ACTUALIZAR:-0}" = "1" ] && exit 0

echo
echo "$pasaron pasaron, $fallaron fallaron"
if [ "$fallaron" -gt 0 ]; then
    printf 'fallaron: %s\n' "${fallidos[*]}"
    exit 1
fi
