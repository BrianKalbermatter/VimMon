#!/bin/bash
# pack.sh — empaqueta PseudoGames para release
# Uso: ejecutar desde la carpeta PseudoGames/
set -e

OUTPUT="pseudogames-linux.tar.gz"

# Crear saves/ vacio si no existe
mkdir -p saves

echo "Empaquetando $OUTPUT..."

tar -czf "../$OUTPUT" \
    --exclude="*Zone.Identifier*" \
    --exclude="*.exe" \
    --transform="s|scripts/launcher.sh|launcher.sh|" \
    aed \
    assets/ \
    data/ \
    saves/ \
    Frankly/ \
    scripts/launcher.sh

echo "Listo: ../$OUTPUT"
echo "Ahora subilo al release con:"
echo "  gh release upload <tag> ../$OUTPUT --clobber"
