#!/bin/bash

# Script d'automation des tests unitaires DPM2_NUCLEO
# Usage: ./run_tests.sh [options]

set -e  # Arrêt sur erreur

# ============================================================================
# CONFIGURATION
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(dirname "$TEST_DIR")"

# Couleurs pour l'affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Options par défaut
RUN_NATIVE=true
RUN_EMBEDDED=false
RUN_STATIC_ANALYSIS=false
RUN_COVERAGE=false
VERBOSE=false
CLEAN_BEFORE=false
GENERATE_REPORTS=true

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
    
    # Vérifier ARM GCC (optionnel pour tests embarqués)
    if [ "$RUN_EMBEDDED" = true ] && ! command -v arm-none-eabi-gcc &> /dev/null; then
        missing_tools+=("arm-none-eabi-gcc")
    fi
    
    # Vérifier Make
    if ! command -v make &> /dev/null; then
        missing_tools+=("make")
    fi
    
    # Vérifier cppcheck (optionnel pour analyse statique)
    if [ "$RUN_STATIC_ANALYSIS" = true ] && ! command -v cppcheck &> /dev/null; then
        print_warning "cppcheck non trouvé - analyse statique désactivée"
        RUN_STATIC_ANALYSIS=false
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
        cd "$TEST_DIR"
        make clean
        print_success "Nettoyage terminé"
    fi
}

# Exécuter les tests natifs
run_native_tests() {
    if [ "$RUN_NATIVE" = true ]; then
        print_header "TESTS NATIFS (PC)"
        cd "$TEST_DIR"
        
        local make_flags=""
        if [ "$VERBOSE" = true ]; then
            make_flags="VERBOSE=1"
        fi
        
        if [ "$RUN_COVERAGE" = true ]; then
            print_info "Exécution avec couverture de code..."
            make coverage $make_flags
        else
            make test-native $make_flags
        fi
        
        print_success "Tests natifs terminés"
    fi
}

# Compiler les tests embarqués
run_embedded_tests() {
    if [ "$RUN_EMBEDDED" = true ]; then
        print_header "TESTS EMBARQUÉS (STM32)"
        cd "$TEST_DIR"
        
        print_info "Compilation des tests embarqués..."
        make test-embedded
        
        print_success "Compilation des tests embarqués terminée"
        print_warning "Flashage et exécution sur cible requis manuellement"
    fi
}

# Analyse statique
run_static_analysis() {
    if [ "$RUN_STATIC_ANALYSIS" = true ]; then
        print_header "ANALYSE STATIQUE"
        cd "$TEST_DIR"
        
        make static-analysis
        print_success "Analyse statique terminée"
    fi
}

# Générer les rapports
generate_reports() {
    if [ "$GENERATE_REPORTS" = true ]; then
        print_header "GÉNÉRATION DES RAPPORTS"
        cd "$TEST_DIR"
        
        make test-reports
        
        # Afficher le résumé
        if [ -d "reports" ]; then
            local total_logs=$(find reports -name "*.log" | wc -l)
            if [ $total_logs -gt 0 ]; then
                print_success "Rapports générés dans: $TEST_DIR/reports/"
                
                # Lister les fichiers de rapport
                echo "Fichiers générés:"
                find reports -type f -name "*.log" -o -name "*.xml" -o -name "*.gcov" | sort | sed 's/^/  - /'
            fi
        fi
    fi
}

# Résumé final
print_summary() {
    print_header "RÉSUMÉ DE L'EXÉCUTION"
    
    echo "Configuration:"
    echo "  - Tests natifs: $([ "$RUN_NATIVE" = true ] && echo "✅" || echo "❌")"
    echo "  - Tests embarqués: $([ "$RUN_EMBEDDED" = true ] && echo "✅" || echo "❌")"
    echo "  - Analyse statique: $([ "$RUN_STATIC_ANALYSIS" = true ] && echo "✅" || echo "❌")"
    echo "  - Couverture: $([ "$RUN_COVERAGE" = true ] && echo "✅" || echo "❌")"
    echo "  - Rapports: $([ "$GENERATE_REPORTS" = true ] && echo "✅" || echo "❌")"
    
    # Analyser les résultats si disponibles
    if [ -d "$TEST_DIR/reports" ]; then
        local log_files=$(find "$TEST_DIR/reports" -name "*.log" 2>/dev/null)
        if [ -n "$log_files" ]; then
            local total_tests=0
            local failed_tests=0
            
            for log_file in $log_files; do
                if [ -f "$log_file" ]; then
                    local tests_in_file=$(grep -c "RUN_TEST" "$log_file" 2>/dev/null || echo 0)
                    local failures_in_file=$(grep -c "FAIL:" "$log_file" 2>/dev/null || echo 0)
                    total_tests=$((total_tests + tests_in_file))
                    failed_tests=$((failed_tests + failures_in_file))
                fi
            done
            
            echo ""
            echo "Résultats des tests:"
            echo "  - Total: $total_tests"
            echo "  - Réussis: $((total_tests - failed_tests))"
            echo "  - Échoués: $failed_tests"
            
            if [ $failed_tests -eq 0 ] && [ $total_tests -gt 0 ]; then
                print_success "Tous les tests sont passés! 🎉"
                exit 0
            elif [ $failed_tests -gt 0 ]; then
                print_error "$failed_tests test(s) ont échoué"
                exit 1
            fi
        fi
    fi
}

# Afficher l'aide
show_help() {
    echo "Tests Unitaires DPM2_NUCLEO"
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -n, --native          Exécuter tests natifs (défaut: activé)"
    echo "  -e, --embedded        Exécuter tests embarqués"
    echo "  -s, --static          Analyse statique avec cppcheck"
    echo "  -c, --coverage        Couverture de code"
    echo "  -v, --verbose         Mode verbeux"
    echo "  --clean               Nettoyer avant exécution"
    echo "  --no-reports          Ne pas générer de rapports"
    echo "  --native-only         Tests natifs uniquement"
    echo "  --all                 Tous les tests et analyses"
    echo "  -h, --help            Afficher cette aide"
    echo ""
    echo "Exemples:"
    echo "  $0                    # Tests natifs basiques"
    echo "  $0 --all              # Tous les tests et analyses"
    echo "  $0 -n -c              # Tests natifs avec couverture"
    echo "  $0 -e -s              # Tests embarqués + analyse statique"
}

# ============================================================================
# TRAITEMENT DES ARGUMENTS
# ============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
        -n|--native)
            RUN_NATIVE=true
            shift
            ;;
        -e|--embedded)
            RUN_EMBEDDED=true
            shift
            ;;
        -s|--static)
            RUN_STATIC_ANALYSIS=true
            shift
            ;;
        -c|--coverage)
            RUN_COVERAGE=true
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
        --no-reports)
            GENERATE_REPORTS=false
            shift
            ;;
        --native-only)
            RUN_NATIVE=true
            RUN_EMBEDDED=false
            RUN_STATIC_ANALYSIS=false
            shift
            ;;
        --all)
            RUN_NATIVE=true
            RUN_EMBEDDED=true
            RUN_STATIC_ANALYSIS=true
            RUN_COVERAGE=true
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
    print_header "TESTS UNITAIRES DPM2_NUCLEO"
    
    # Vérifications préliminaires
    check_prerequisites
    
    # Nettoyage si demandé
    clean_build
    
    # Exécution des tests
    run_native_tests
    run_embedded_tests
    
    # Analyses
    run_static_analysis
    
    # Rapports
    generate_reports
    
    # Résumé
    print_summary
}

# Gestion des erreurs
trap 'print_error "Erreur détectée à la ligne $LINENO. Arrêt du script."; exit 1' ERR

# Exécution
main "$@"
