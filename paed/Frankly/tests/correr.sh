#!/usr/bin/env bash
# Corre cada programa .paed de esta carpeta y compara su salida contra la que
# el propio archivo declara al final, en un bloque de comentarios:
#
#     FIN_ACCION
#
#     // ── SALIDA ESPERADA ──
#     // 1..10 suman 55
#     // Gauss tenia razon
#
# Un test es UN archivo. Antes la salida vivía en un .esperado al lado, y eso
# tenía dos problemas: duplicaba archivos, y la referencia quedaba lejos del
# código que la produce.
#
# NO existe un modo que regrabe la salida sola. Si un test falla, el runner
# muestra el diff y el bloque se corrige A MANO. Regrabar sin leer es cómo un
# test deja de proteger: "arregla" el test en vez del bug.
#
#   bash paed/Frankly/tests/correr.sh
#
# Agregar un test = dejar el .paed con su bloque al final. Nada más: no hay
# ninguna lista que mantener.

set -uo pipefail

MARCA='// ── SALIDA ESPERADA'

# La raíz del repo: dos niveles arriba de paed/Frankly/tests
raiz=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)
cd "$raiz" || exit 1

runner=build/paedrun
if [ ! -x "$runner" ]; then
    echo "falta $runner — corré 'make paedrun' primero" >&2
    exit 1
fi

tests_dir=paed/Frankly/tests
pasaron=0
fallaron=0
fallidos=()

# Saca el bloque de salida esperada de un .paed: desde la marca hasta el final,
# quitando la marca y el '// ' de cada línea.
#
# Va con awk y no con sed porque la marca EMPIEZA con '//', y esas barras le
# cierran el patrón a sed en la cara.
esperado_de() {
    awk -v m="$MARCA" 'index($0, m) == 1 { visto = 1; next } visto' "$1" \
        | sed 's|^// \?||'
}

for prog in "$tests_dir"/*.paed; do
    [ -e "$prog" ] || continue
    nombre=$(basename "$prog")

    if ! grep -q "^${MARCA}" "$prog"; then
        echo "  SIN BLOQUE   $nombre (falta '${MARCA} ──' al final)"
        fallaron=$((fallaron + 1))
        fallidos+=("$nombre")
        continue
    fi

    # stderr se junta con stdout: los mensajes de error son parte de lo que se
    # verifica. Un error con el texto equivocado es un test fallado.
    real=$("$runner" "$prog" 2>&1)

    if [ "$real" = "$(esperado_de "$prog")" ]; then
        echo "  ok       $nombre"
        pasaron=$((pasaron + 1))
    else
        echo "  FALLA    $nombre"
        diff -u <(esperado_de "$prog") <(printf '%s\n' "$real") \
            | sed 's/^/           /'
        fallaron=$((fallaron + 1))
        fallidos+=("$nombre")
    fi
done

echo
echo "$pasaron pasaron, $fallaron fallaron"
if [ "$fallaron" -gt 0 ]; then
    printf 'fallaron: %s\n' "${fallidos[*]}"
    echo "corregí el bloque SALIDA ESPERADA del .paed a mano, leyendo el diff"
    exit 1
fi
