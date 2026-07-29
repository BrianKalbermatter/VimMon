#!/usr/bin/env bash
# watch.sh — vigila todo el arbol de game/ y recompila game.so en cuanto algo
# cambia. Cualquier .c/.h que agregues ahi adentro entra solo.
#
# Esto es lo que hace por vos el plugin de hot-reload de Unity: mirar los
# archivos y compilar. No hay magia, solo un trigger automatico.
#
# No hay lista de exclusiones: lo que no esta en game/ (host.c, lab/, docs)
# simplemente no se vigila. Mismo criterio que GAME_DIR en el Makefile.
#
# El host nunca se toca: queda corriendo en otro panel y detecta el game.so
# nuevo por su mtime.
#
# Uso: ./watch.sh   (siempre vigila el conjunto entero, no un archivo suelto)

set -u

GAME_DIR=game

echo "[watch] vigilando $GAME_DIR/ (Ctrl-C para cortar)"

anterior=""
while true; do
  # mtime mas reciente entre todos los .c/.h del juego.
  actual=$(find "$GAME_DIR" \( -name '*.c' -o -name '*.h' \) \
    | xargs -r stat -c %Y 2>/dev/null \
    | sort -n | tail -1)

  if [ -n "$actual" ] && [ "$actual" != "$anterior" ]; then
    anterior="$actual"
    printf '\n[watch] cambio detectado, compilando...\n'
    if make game.so; then
      echo "[watch] listo — el host lo levanta en medio segundo"
    else
      echo "[watch] no compila: game.so quedo como estaba, segui jugando"
    fi
  fi

  sleep 0.3
done
