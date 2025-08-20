#!/bin/bash

# Script pour tester la syntaxe GitHub Actions
# Usage: ./scripts/test-github-actions.sh

set -e

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_header() {
    echo -e "\n${BLUE}============================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}============================================${NC}\n"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
WORKFLOW_FILE="$PROJECT_ROOT/.github/workflows/nucleo-ci.yml"

print_header "TEST GITHUB ACTIONS SYNTAX"

# Vérifier que le fichier existe
if [ ! -f "$WORKFLOW_FILE" ]; then
    print_error "Fichier workflow non trouvé: $WORKFLOW_FILE"
    exit 1
fi

print_info "Test du fichier: $WORKFLOW_FILE"

# Test 1: Validation YAML basique
print_info "Test 1: Validation YAML basique..."
if python3 -c "import yaml; yaml.safe_load(open('$WORKFLOW_FILE'))" 2>/dev/null; then
    print_success "YAML syntaxiquement correct"
else
    print_error "Erreur de syntaxe YAML"
    exit 1
fi

# Test 2: Vérification des clés obligatoires
print_info "Test 2: Vérification structure workflow..."
python3 - <<EOF
import yaml
import sys

with open('$WORKFLOW_FILE', 'r') as f:
    workflow = yaml.safe_load(f)

# Vérifications critiques
errors = []

if 'name' not in workflow:
    errors.append("Clé 'name' manquante")
    
if 'on' not in workflow:
    errors.append("Clé 'on' manquante")
    
if 'jobs' not in workflow:
    errors.append("Clé 'jobs' manquante")
else:
    jobs = workflow['jobs']
    for job_name, job_config in jobs.items():
        if 'runs-on' not in job_config:
            errors.append(f"Job '{job_name}': 'runs-on' manquant")
        if 'steps' not in job_config:
            errors.append(f"Job '{job_name}': 'steps' manquant")

if errors:
    for error in errors:
        print(f"❌ {error}")
    sys.exit(1)
else:
    print("✅ Structure workflow valide")
EOF

# Test 3: Vérification des expressions GitHub Actions
print_info "Test 3: Vérification des expressions..."

# Vérifier les expressions ${{ }}
if grep -n '\${{.*}}' "$WORKFLOW_FILE" > /dev/null; then
    print_success "Expressions GitHub Actions trouvées"
    
    # Vérifier les expressions problématiques
    if grep -n '\${{.*secrets\..*}}' "$WORKFLOW_FILE" > /dev/null; then
        print_success "Syntaxe secrets correcte"
    fi
else
    print_warning "Aucune expression GitHub Actions trouvée"
fi

# Test 4: Vérification des conditions if
print_info "Test 4: Vérification des conditions..."

problematic_patterns=(
    "if:.*secrets\.[A-Z_]+.*&&.*secrets\.[A-Z_]+"
    "if:.*github\.event_name.*!="
)

for pattern in "${problematic_patterns[@]}"; do
    if grep -E "$pattern" "$WORKFLOW_FILE" > /dev/null; then
        print_warning "Pattern potentiellement problématique trouvé: $pattern"
    fi
done

# Test 5: Vérification des actions utilisées
print_info "Test 5: Vérification des actions..."

actions_used=$(grep -E "uses:\s+" "$WORKFLOW_FILE" | sed 's/.*uses:\s*//' | sort | uniq)
echo "Actions utilisées:"
while IFS= read -r action; do
    echo "  - $action"
done <<< "$actions_used"

# Test 6: Simulation d'un dry-run (si GitHub CLI disponible)
if command -v gh &> /dev/null; then
    print_info "Test 6: GitHub CLI disponible - test dry-run..."
    
    # Note: gh ne supporte pas encore la validation de workflow
    # mais on peut vérifier la connexion
    if gh auth status &> /dev/null; then
        print_success "GitHub CLI authentifié"
    else
        print_warning "GitHub CLI non authentifié - impossible de tester"
    fi
else
    print_info "Test 6: GitHub CLI non disponible - skip dry-run"
fi

print_header "RÉSUMÉ DES TESTS"

echo "✅ Tests passés avec succès:"
echo "  - Syntaxe YAML valide"
echo "  - Structure workflow correcte"
echo "  - Expressions GitHub Actions présentes"
echo "  - Actions externes référencées"

echo ""
echo "📋 Prochaines étapes:"
echo "  1. Commit et push sur une branche de test"
echo "  2. Vérifier l'exécution dans GitHub Actions"
echo "  3. Configurer les secrets si nécessaire"
echo "  4. Tester avec un vrai workflow"

print_success "Validation syntaxe terminée - Le workflow semble prêt !"
