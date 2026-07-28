#!/usr/bin/env bash
# watch.sh — vigila el fuente del juego y recompila solo cuando cambia.
#
# Esto es lo que hace por vos el plugin de hot-reload de Unity: mirar el
# archivo y compilar. No hay magia, solo un trigger automatico.
#
# El host nunca se toca: queda corriendo en otro panel y detecta el game.so
# nuevo por su mtime.
#
# Uso:  ./watch.sh            (vigila demo.c)
#       ./watch.sh dungeon.c  (vigila tu juego)

set -u

FUENTE="${1:-demo.c}"

# Cada fuente tiene su target en el Makefile.
if [ "$FUENTE" = "demo.c" ]; then
  TARGET="demo"
else
  TARGET="game.so"
fi

if [ ! -f "$FUENTE" ]; then
  echo "[watch] no existe $FUENTE" >&2
  exit 1
fi

echo "[watch] vigilando $FUENTE -> $TARGET (Ctrl-C para cortar)"

anterior=""
while true; do
  actual=$(stat -c %Y "$FUENTE" 2>/dev/null || echo "")

  if [ -n "$actual" ] && [ "$actual" != "$anterior" ]; then
    anterior="$actual"
    printf '\n[watch] %s cambio, compilando...\n' "$FUENTE"
    if make "$TARGET"; then
      echo "[watch] listo — el host lo levanta en medio segundo"
    else
      echo "[watch] no compila: game.so quedo como estaba, segui jugando"
    fi
  fi

  sleep 0.3
done
