#!/bin/bash
# pack.sh — empaqueta PseudoGames para release
# Uso: ejecutar desde la carpeta PseudoGames/
set -e

OUTPUT="pseudogames-linux.tar.gz"

echo "Empaquetando $OUTPUT..."

tar -czf "../$OUTPUT" \
    aed \
    assets/ \
    data/ \
    Frankly/ \
    scripts/launcher.sh

echo "Listo: ../$OUTPUT"
echo "Ahora subilo al release con:"
echo "  gh release upload <tag> ../$OUTPUT --clobber"
