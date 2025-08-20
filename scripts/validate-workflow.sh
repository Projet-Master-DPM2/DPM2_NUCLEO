#!/bin/bash

# Script pour valider la syntaxe du workflow GitHub Actions
# Usage: ./scripts/validate-workflow.sh

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

print_header "VALIDATION WORKFLOW GITHUB ACTIONS"

# Vérifier que le fichier existe
if [ ! -f "$WORKFLOW_FILE" ]; then
    print_error "Fichier workflow non trouvé: $WORKFLOW_FILE"
    exit 1
fi

print_info "Validation du fichier: $WORKFLOW_FILE"

# Vérifier Python3
if ! command -v python3 &> /dev/null; then
    print_error "Python3 requis pour la validation"
    exit 1
fi

# Validation YAML avec Python
print_info "Vérification de la syntaxe YAML..."

cat > /tmp/validate_workflow.py <<'EOF'
import yaml
import sys
import os

workflow_file = sys.argv[1] if len(sys.argv) > 1 else '.github/workflows/nucleo-ci.yml'

try:
    with open(workflow_file, 'r') as f:
        workflow = yaml.safe_load(f)
    
    # Vérifications de base
    required_keys = ['name', 'on', 'jobs']
    for key in required_keys:
        if key not in workflow:
            print(f"❌ Clé manquante: {key}")
            sys.exit(1)
    
    # Vérifier les jobs
    jobs = workflow.get('jobs', {})
    expected_jobs = ['native-tests', 'cppcheck', 'build-nucleo', 'release', 'email']
    
    for job in expected_jobs:
        if job not in jobs:
            print(f"⚠️  Job manquant: {job}")
        else:
            job_config = jobs[job]
            if 'runs-on' not in job_config:
                print(f"❌ Job {job}: 'runs-on' manquant")
                sys.exit(1)
            if 'steps' not in job_config:
                print(f"❌ Job {job}: 'steps' manquant")
                sys.exit(1)
    
    print("✅ Syntaxe YAML valide")
    print(f"✅ {len(jobs)} jobs définis")
    
    # Vérifier les triggers
    triggers = workflow.get('on', {})
    if isinstance(triggers, dict):
        if 'push' in triggers:
            push_config = triggers['push']
            if 'branches' in push_config:
                print(f"✅ Push branches: {push_config['branches']}")
            if 'tags' in push_config:
                print(f"✅ Push tags: {push_config['tags']}")
        if 'pull_request' in triggers:
            print("✅ Pull request trigger configuré")
    
    print("✅ Configuration workflow valide")
    
except yaml.YAMLError as e:
    print(f"❌ Erreur YAML: {e}")
    sys.exit(1)
except FileNotFoundError:
    print(f"❌ Fichier non trouvé: {workflow_file}")
    sys.exit(1)
except Exception as e:
    print(f"❌ Erreur: {e}")
    sys.exit(1)
EOF

python3 /tmp/validate_workflow.py "$WORKFLOW_FILE"

# Vérifications spécifiques au projet NUCLEO
print_info "Vérifications spécifiques NUCLEO..."

# Vérifier que les paths sont corrects
if grep -q "DPM2_NUCLEO/test" "$WORKFLOW_FILE"; then
    print_success "Paths de tests corrects"
else
    print_warning "Vérifiez les paths vers DPM2_NUCLEO/test"
fi

# Vérifier les includes STM32
if grep -q "STM32F4xx_HAL_Driver" "$WORKFLOW_FILE"; then
    print_success "Includes STM32 HAL configurés"
else
    print_warning "Includes STM32 HAL manquants"
fi

# Vérifier les artefacts
if grep -q "nucleo-firmware" "$WORKFLOW_FILE"; then
    print_success "Nom d'artefact NUCLEO configuré"
else
    print_warning "Nom d'artefact à vérifier"
fi

# Suggestions d'amélioration
print_header "SUGGESTIONS ET VÉRIFICATIONS"

echo "📋 Checklist avant utilisation:"
echo "  □ Configurer les secrets GitHub (SMTP_*, MAIL_*)"
echo "  □ Vérifier les permissions du repository"
echo "  □ Tester avec un push sur une branche de test"
echo "  □ Vérifier que les paths vers les sources sont corrects"
echo "  □ S'assurer que Unity est bien configuré dans test/"

echo ""
echo "🔧 Améliorations possibles:"
echo "  - Ajouter cache pour les dépendances ARM"
echo "  - Configurer matrix builds pour différentes configs"
echo "  - Ajouter step de validation des artefacts"
echo "  - Intégrer coverage reports"

echo ""
print_info "Pour tester localement:"
echo "  ./scripts/ci-local.sh --all"

print_success "Validation workflow terminée"

# Nettoyer
rm -f /tmp/validate_workflow.py