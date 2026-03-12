#!/bin/bash
# release.sh — compila, empaqueta y sube una nueva release a GitHub
# Uso: ./scripts/release.sh v0.3

set -e  # parar si algo falla

VERSION=${1:-""}
if [ -z "$VERSION" ]; then
    echo "Uso: $0 <version>  (ejemplo: $0 v0.3)"
    exit 1
fi

REPO="BrianKalbermatter/VimMon"
ZIP="PseudoGames_${VERSION}.zip"

echo "==> Compilando para Windows..."
touch src/*.c
make windows

echo "==> Empaquetando ${ZIP}..."
python3 -c "
import zipfile, os
with zipfile.ZipFile('${ZIP}', 'w', zipfile.ZIP_DEFLATED) as z:
    z.write('PseudoGames.exe')
    z.write('win-libs/dll/SDL2_ttf.dll', 'SDL2_ttf.dll')
print(f'  ZIP creado: {os.path.getsize(\"${ZIP}\") // 1024} KB')
"

echo "==> Subiendo exe al repo..."
git add -f PseudoGames.exe
git commit -m "build: release ${VERSION}" || echo "  (nada nuevo para commitear)"
git push origin aed_pseudo

echo "==> Borrando release anterior con el mismo tag (si existe)..."
gh release delete "${VERSION}" --repo "${REPO}" --yes 2>/dev/null || true
git tag -d "${VERSION}" 2>/dev/null || true
git push origin ":refs/tags/${VERSION}" 2>/dev/null || true

echo "==> Creando release ${VERSION} en GitHub..."
gh release create "${VERSION}" "${ZIP}" \
    --repo "${REPO}" \
    --title "PseudoGames ${VERSION}" \
    --notes "## Instalacion
Descomprimí el ZIP en cualquier carpeta y ejecutá PseudoGames.exe.
Solo necesita SDL2_ttf.dll en la misma carpeta. Sin instalacion." \
    --target aed_pseudo

echo ""
echo "✓ Release ${VERSION} publicada:"
gh release view "${VERSION}" --repo "${REPO}" --json url -q .url

# Limpiar ZIP local
rm -f "${ZIP}"
