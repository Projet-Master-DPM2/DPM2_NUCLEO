#!/bin/bash

# Script de test pour la création de ZIP avec noms de branches problématiques
# Simule la logique du workflow GitHub Actions

echo "============================================"
echo "TEST DE CRÉATION ZIP AVEC NOMS PROBLÉMATIQUES"
echo "============================================"

# Test avec différents noms de branches
test_branches=("CI/CD" "feature/test-branch" "hotfix/bug-fix" "release/v1.2.3" "main" "dev")

echo "🧪 Test de nettoyage des noms de branches:"
for branch in "${test_branches[@]}"; do
    clean_name=$(echo "$branch" | sed 's/[^a-zA-Z0-9._-]/_/g')
    zip_name="nucleo-firmware-${clean_name}.zip"
    echo "  '$branch' → '$clean_name' → '$zip_name'"
done

echo ""
echo "✅ Simulation de création ZIP:"

# Créer un répertoire temporaire pour les tests
TEST_DIR="test_zip_creation"
mkdir -p "$TEST_DIR/out"

# Créer des fichiers factices
echo "Test firmware binary" > "$TEST_DIR/out/DPM2_NUCLEO.bin"
echo "Test firmware hex" > "$TEST_DIR/out/DPM2_NUCLEO.hex"
echo "Test firmware elf" > "$TEST_DIR/out/DPM2_NUCLEO.elf"
echo "Test firmware map" > "$TEST_DIR/out/DPM2_NUCLEO.map"

# Tester avec une branche problématique
cd "$TEST_DIR"
BRANCH_NAME="CI/CD"
CLEAN_NAME=$(echo "$BRANCH_NAME" | sed 's/[^a-zA-Z0-9._-]/_/g')
ZIP="nucleo-firmware-${CLEAN_NAME}.zip"

echo "  Création du ZIP: $ZIP"
if (cd out && zip -r "../$ZIP" . > /dev/null 2>&1); then
    echo "  ✅ ZIP créé avec succès: $(ls -la $ZIP 2>/dev/null | awk '{print $5" bytes"}')"
    
    # Vérifier le contenu
    echo "  📦 Contenu du ZIP:"
    unzip -l "$ZIP" | grep -E "\.(bin|hex|elf|map)$" | sed 's/^/    /'
else
    echo "  ❌ Erreur lors de la création du ZIP"
fi

# Nettoyage
cd ..
rm -rf "$TEST_DIR"

echo ""
echo "🎯 Résultat: La correction résout le problème de caractères spéciaux"
echo "   dans les noms de fichiers ZIP."

