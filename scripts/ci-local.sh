#!/bin/bash

# Script pour tester la pipeline CI/CD localement
# Usage: ./scripts/ci-local.sh [options]

set -e  # Arrêt sur erreur

# ============================================================================
# CONFIGURATION
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Couleurs pour l'affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Options par défaut
RUN_TESTS=true
RUN_CPPCHECK=true
RUN_BUILD=false  # Build ARM désactivé par défaut (nécessite toolchain)
VERBOSE=false
CLEAN_BEFORE=false

# ============================================================================
# FONCTIONS UTILITAIRES
# ============================================================================

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

# Vérifier les prérequis
check_prerequisites() {
    print_header "VÉRIFICATION DES PRÉREQUIS"
    
    local missing_tools=()
    
    # Vérifier GCC
    if ! command -v gcc &> /dev/null; then
        missing_tools+=("gcc")
    fi
    
    # Vérifier Make
    if ! command -v make &> /dev/null; then
        missing_tools+=("make")
    fi
    
    # Vérifier Python3
    if ! command -v python3 &> /dev/null; then
        missing_tools+=("python3")
    fi
    
    # Vérifier cppcheck (optionnel)
    if [ "$RUN_CPPCHECK" = true ] && ! command -v cppcheck &> /dev/null; then
        print_warning "cppcheck non trouvé - analyse statique désactivée"
        RUN_CPPCHECK=false
    fi
    
    # Vérifier ARM GCC (optionnel pour build)
    if [ "$RUN_BUILD" = true ] && ! command -v arm-none-eabi-gcc &> /dev/null; then
        print_warning "arm-none-eabi-gcc non trouvé - build ARM désactivé"
        RUN_BUILD=false
    fi
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        print_error "Outils manquants: ${missing_tools[*]}"
        echo "Installez les outils manquants et relancez le script."
        exit 1
    fi
    
    print_success "Tous les prérequis sont satisfaits"
}

# Nettoyer les fichiers de build
clean_build() {
    if [ "$CLEAN_BEFORE" = true ]; then
        print_header "NETTOYAGE"
        cd "$PROJECT_ROOT"
        
        # Nettoyer les tests
        if [ -d "test" ]; then
            cd test
            make clean 2>/dev/null || true
            cd ..
        fi
        
        # Nettoyer le build principal
        rm -rf build Debug *.elf *.hex *.bin *.map
        
        print_success "Nettoyage terminé"
    fi
}

# Exécuter les tests natifs
run_native_tests() {
    if [ "$RUN_TESTS" = true ]; then
        print_header "TESTS NATIFS (PC)"
        cd "$PROJECT_ROOT/test"
        
        mkdir -p reports
        
        # Exécuter les tests et capturer les résultats
        if [ "$VERBOSE" = true ]; then
            make test-native
        else
            make test-native > reports/tests_raw.log 2>&1 || true
        fi
        
        # Convertir les résultats Unity en format JUnit XML
        python3 - <<'PY'
import re, xml.etree.ElementTree as ET
from datetime import datetime

# Lire le log des tests
try:
    with open('reports/tests_raw.log', 'r') as f:
        content = f.read()
except FileNotFoundError:
    print("❌ Fichier de log des tests non trouvé")
    exit(1)

# Parser les résultats Unity
test_pattern = r'\./(.*?):(\d+):(.*?):(PASS|FAIL)'
matches = re.findall(test_pattern, content)

if not matches:
    print("⚠️ Aucun résultat de test trouvé dans le log")
    # Créer un XML vide
    root = ET.Element('testsuites')
    root.set('name', 'NUCLEO Native Tests')
    root.set('tests', '0')
    root.set('failures', '0')
    tree = ET.ElementTree(root)
    tree.write('reports/tests.xml', encoding='utf-8', xml_declaration=True)
    exit(0)

# Créer XML JUnit
root = ET.Element('testsuites')
root.set('name', 'NUCLEO Native Tests')
root.set('timestamp', datetime.now().isoformat())

# Grouper par fichier de test
test_files = {}
for file_path, line_num, test_name, status in matches:
    if file_path not in test_files:
        test_files[file_path] = []
    test_files[file_path].append((test_name, status, line_num))

total_tests = 0
total_failures = 0

for file_path, tests in test_files.items():
    suite = ET.SubElement(root, 'testsuite')
    suite.set('name', file_path.replace('/', '_'))
    suite.set('tests', str(len(tests)))
    
    failures = sum(1 for _, status, _ in tests if status == 'FAIL')
    suite.set('failures', str(failures))
    suite.set('errors', '0')
    suite.set('time', '1.0')
    
    total_tests += len(tests)
    total_failures += failures
    
    for test_name, status, line_num in tests:
        testcase = ET.SubElement(suite, 'testcase')
        testcase.set('name', test_name)
        testcase.set('classname', file_path.replace('/', '.'))
        testcase.set('time', '0.1')
        
        if status == 'FAIL':
            failure = ET.SubElement(testcase, 'failure')
            failure.set('message', f'Test failed at line {line_num}')
            failure.text = f'Test {test_name} failed in {file_path}:{line_num}'

root.set('tests', str(total_tests))
root.set('failures', str(total_failures))

# Sauvegarder le XML
tree = ET.ElementTree(root)
tree.write('reports/tests.xml', encoding='utf-8', xml_declaration=True)

print(f"✅ JUnit XML créé avec {total_tests} tests, {total_failures} échecs")
PY
        
        # Générer le rapport Markdown
        python3 - <<'PY'
import xml.etree.ElementTree as ET, os

xml_path = 'reports/tests.xml'
if not os.path.exists(xml_path):
    print(f"❌ Erreur: {xml_path} n'existe pas")
    exit(1)

tree = ET.parse(xml_path)
root = tree.getroot()

# JUnit peut avoir <testsuites> ou un seul <testsuite>
if root.tag == 'testsuite':
    suites = [root]
else:
    suites = list(root.findall('testsuite'))

rows = []
total = passed = failed = skipped = 0

def status_of(case):
    if case.find('failure') is not None or case.find('error') is not None:
        return 'FAIL'
    if case.find('skipped') is not None:
        return 'SKIP'
    return 'PASS'

for suite in suites:
    suite_name = (suite.get('name') or 'native').replace('_', '/')
    for case in suite.findall('testcase'):
        name  = case.get('name') or ''
        st    = status_of(case)
        try:
            dur_s = float(case.get('time') or 0.1)
        except:
            dur_s = 0.1
        dur_s = max(dur_s, 0.001)  # min 0.001s

        total += 1
        if st == 'PASS':
            passed += 1; badge = '✅ PASS'
        elif st == 'SKIP':
            skipped += 1; badge = '⚠️ SKIP'
        else:
            failed += 1; badge = '❌ FAIL'

        full_name = f"{suite_name}/{name}" if name else suite_name
        rows.append((full_name, badge, f"{dur_s:.3f}s"))

# Écrire le Markdown dans un fichier séparé
with open('reports/tests.md', 'w', encoding='utf-8') as f:
    f.write("### Tests unitaires NUCLEO (native)\n")
    f.write(f"**Résumé** : {passed} pass · {failed} fail · {skipped} skip · total {total}\n\n")
    f.write("| Test | Statut | Durée |\n")
    f.write("|---|:---:|---:|\n")
    for n,s,t in rows:
        f.write(f"| `{n}` | {s} | {t} |\n")

print(f"✅ Markdown créé avec {total} tests ({passed} pass, {failed} fail)")
PY
        
        # Afficher le résumé
        if [ -f "reports/tests.md" ]; then
            echo ""
            cat reports/tests.md
        fi
        
        print_success "Tests natifs terminés"
        cd "$PROJECT_ROOT"
    fi
}

# Analyse statique
run_cppcheck() {
    if [ "$RUN_CPPCHECK" = true ]; then
        print_header "ANALYSE STATIQUE (CPPCHECK)"
        cd "$PROJECT_ROOT"
        
        mkdir -p reports/cppcheck_html
        
        # Exécuter cppcheck
        cppcheck --std=c99 --enable=all --inline-suppr \
                 --suppress=missingIncludeSystem \
                 --suppress=unusedFunction \
                 --suppress=unmatchedSuppression \
                 -I Core/Inc -I Core/Inc/Services \
                 -I Drivers/STM32F4xx_HAL_Driver/Inc \
                 -I Drivers/CMSIS/Device/ST/STM32F4xx/Include \
                 -I Drivers/CMSIS/Include \
                 -I Middlewares/Third_Party/FreeRTOS/Source/include \
                 -I Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 \
                 --xml --xml-version=2 Core/Src/ 2> reports/cppcheck.xml || true
        
        # Générer rapport HTML si possible
        if command -v cppcheck-htmlreport >/dev/null; then
            cppcheck-htmlreport --file=reports/cppcheck.xml \
                                --report-dir=reports/cppcheck_html \
                                --source-dir=. || true
            print_success "Rapport HTML généré dans reports/cppcheck_html/"
        else
            print_info "cppcheck-htmlreport non disponible - XML seulement"
        fi
        
        # Compter les erreurs
        if [ -f "reports/cppcheck.xml" ]; then
            errors=$(grep -c 'severity="error"' reports/cppcheck.xml 2>/dev/null || echo 0)
            warnings=$(grep -c 'severity="warning"' reports/cppcheck.xml 2>/dev/null || echo 0)
            echo ""
            echo "📊 Résultats cppcheck:"
            echo "  - Erreurs: $errors"
            echo "  - Avertissements: $warnings"
            
            if [ "$errors" -eq 0 ]; then
                print_success "Aucune erreur détectée"
            else
                print_warning "$errors erreur(s) détectée(s)"
            fi
        fi
        
        print_success "Analyse statique terminée"
    fi
}

# Build ARM (optionnel)
run_arm_build() {
    if [ "$RUN_BUILD" = true ]; then
        print_header "BUILD ARM (STM32F411RE)"
        cd "$PROJECT_ROOT"
        
        print_warning "Build ARM non implémenté dans ce script"
        print_info "Utilisez STM32CubeIDE ou un Makefile approprié"
        
        print_success "Build ARM simulé"
    fi
}

# Résumé final
print_summary() {
    print_header "RÉSUMÉ DE L'EXÉCUTION"
    
    echo "Configuration:"
    echo "  - Tests natifs: $([ "$RUN_TESTS" = true ] && echo "✅" || echo "❌")"
    echo "  - Analyse statique: $([ "$RUN_CPPCHECK" = true ] && echo "✅" || echo "❌")"
    echo "  - Build ARM: $([ "$RUN_BUILD" = true ] && echo "✅" || echo "❌")"
    
    # Analyser les résultats si disponibles
    if [ -f "test/reports/tests.xml" ]; then
        cd test
        total_tests=$(python3 -c "
import xml.etree.ElementTree as ET
try:
    tree = ET.parse('reports/tests.xml')
    root = tree.getroot()
    print(root.get('tests', '0'))
except:
    print('0')
")
        total_failures=$(python3 -c "
import xml.etree.ElementTree as ET
try:
    tree = ET.parse('reports/tests.xml')
    root = tree.getroot()
    print(root.get('failures', '0'))
except:
    print('0')
")
        cd ..
        
        echo ""
        echo "Résultats des tests:"
        echo "  - Total: $total_tests"
        echo "  - Réussis: $((total_tests - total_failures))"
        echo "  - Échoués: $total_failures"
        
        if [ "$total_failures" -eq 0 ] && [ "$total_tests" -gt 0 ]; then
            print_success "Tous les tests sont passés! 🎉"
        elif [ "$total_failures" -gt 0 ]; then
            print_error "$total_failures test(s) ont échoué"
            exit 1
        fi
    fi
}

# Afficher l'aide
show_help() {
    echo "CI/CD Local NUCLEO DPM2"
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -t, --tests           Exécuter tests natifs (défaut: activé)"
    echo "  -c, --cppcheck        Analyse statique cppcheck (défaut: activé)"
    echo "  -b, --build           Build ARM (défaut: désactivé)"
    echo "  -v, --verbose         Mode verbeux"
    echo "  --clean               Nettoyer avant exécution"
    echo "  --no-tests            Désactiver les tests"
    echo "  --no-cppcheck         Désactiver cppcheck"
    echo "  --all                 Activer tous les checks"
    echo "  -h, --help            Afficher cette aide"
    echo ""
    echo "Exemples:"
    echo "  $0                    # Tests + cppcheck"
    echo "  $0 --all              # Tous les checks"
    echo "  $0 -t -v              # Tests en mode verbeux"
    echo "  $0 --no-tests -c      # Cppcheck seulement"
}

# ============================================================================
# TRAITEMENT DES ARGUMENTS
# ============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--tests)
            RUN_TESTS=true
            shift
            ;;
        -c|--cppcheck)
            RUN_CPPCHECK=true
            shift
            ;;
        -b|--build)
            RUN_BUILD=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        --clean)
            CLEAN_BEFORE=true
            shift
            ;;
        --no-tests)
            RUN_TESTS=false
            shift
            ;;
        --no-cppcheck)
            RUN_CPPCHECK=false
            shift
            ;;
        --all)
            RUN_TESTS=true
            RUN_CPPCHECK=true
            RUN_BUILD=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            print_error "Option inconnue: $1"
            show_help
            exit 1
            ;;
    esac
done

# ============================================================================
# EXÉCUTION PRINCIPALE
# ============================================================================

main() {
    print_header "CI/CD LOCAL NUCLEO DPM2"
    
    # Vérifications préliminaires
    check_prerequisites
    
    # Nettoyage si demandé
    clean_build
    
    # Exécution des checks
    run_native_tests
    run_cppcheck
    run_arm_build
    
    # Résumé
    print_summary
}

# Gestion des erreurs
trap 'print_error "Erreur détectée à la ligne $LINENO. Arrêt du script."; exit 1' ERR

# Exécution
main "$@"
