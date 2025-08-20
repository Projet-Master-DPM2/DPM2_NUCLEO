#!/bin/bash

# Script pour simuler exactement le comportement de GitHub Actions
# Usage: ./scripts/simulate-ci.sh

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

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

print_header "SIMULATION CI/CD GITHUB ACTIONS"

cd "$PROJECT_ROOT"

# Étape 1: Setup Unity (comme dans le workflow)
print_info "1. Setup Unity Test Framework..."
cd test

if [ ! -f "unity/unity.h" ] || [ ! -f "unity/unity.c" ]; then
    echo "Unity manquant, téléchargement..."
    mkdir -p unity
    curl -L https://github.com/ThrowTheSwitch/Unity/raw/master/src/unity.h -o unity/unity.h
    curl -L https://github.com/ThrowTheSwitch/Unity/raw/master/src/unity.c -o unity/unity.c
    curl -L https://github.com/ThrowTheSwitch/Unity/raw/master/src/unity_internals.h -o unity/unity_internals.h
else
    echo "Unity déjà présent"
fi

# Étape 2: Exécuter les tests (comme dans le workflow)
print_info "2. Exécution des tests..."
mkdir -p reports

echo "=== Début des tests NUCLEO ===" > reports/tests_raw.log
make test-native >> reports/tests_raw.log 2>&1 || true
echo "=== Fin des tests NUCLEO ===" >> reports/tests_raw.log

echo "Contenu du log de tests:"
cat reports/tests_raw.log

# Étape 3: Conversion JUnit (comme dans le workflow)
print_info "3. Conversion vers JUnit XML..."

python3 - <<'PY'
import re, xml.etree.ElementTree as ET
from datetime import datetime

# Lire le log des tests
with open('reports/tests_raw.log', 'r') as f:
    content = f.read()

print("=== Debug: Contenu du fichier ===")
print(repr(content[:500]))  # Premiers 500 caractères pour debug
print("=== Fin debug ===")

# Parser les résultats Unity - pattern correct observé
unity_pattern = r'\./(.*?):(\d+):(.*?):(PASS|FAIL)'
unity_matches = re.findall(unity_pattern, content)

# Parser les résultats de résumé Makefile
makefile_success_pattern = r'✅ (.*?): SUCCÈS'
makefile_fail_pattern = r'❌ (.*?): ÉCHEC'

success_tests = re.findall(makefile_success_pattern, content)
failed_tests = re.findall(makefile_fail_pattern, content)

print(f"Unity matches: {len(unity_matches)}")
print(f"Makefile success: {len(success_tests)}")
print(f"Makefile failed: {len(failed_tests)}")

# Si on a des résultats Unity détaillés, les utiliser
if unity_matches:
    matches = unity_matches
    print("Utilisation des résultats Unity détaillés")
else:
    # Sinon, créer des matches basés sur les résultats Makefile
    matches = []
    for test_name in success_tests:
        matches.append((f"test/{test_name}", "1", test_name, "PASS"))
    for test_name in failed_tests:
        matches.append((f"test/{test_name}", "1", test_name, "FAIL"))
    print("Utilisation des résultats Makefile")

print(f"Total matches: {len(matches)}")
for match in matches[:5]:
    print(f"Match: {match}")

if not matches:
    print("⚠️ Aucun résultat de test trouvé dans le log")
    print("Contenu complet du log pour debug:")
    print(content)
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

print(f"JUnit XML créé avec {total_tests} tests, {total_failures} échecs")
PY

# Étape 4: Vérification
print_info "4. Vérification des résultats..."

if [[ -f reports/tests.xml ]]; then
    echo "✅ JUnit XML créé: $(wc -l < reports/tests.xml) lignes"
    head -5 reports/tests.xml
else
    print_error "❌ Erreur: reports/tests.xml n'a pas été créé"
    ls -la reports/ || echo "Le dossier reports/ n'existe pas"
    exit 1
fi

# Étape 5: Génération Markdown
print_info "5. Génération rapport Markdown..."

python3 - <<'PY'
import xml.etree.ElementTree as ET, os

xml_path = 'reports/tests.xml'
if not os.path.exists(xml_path):
    print(f"Erreur: {xml_path} n'existe pas")
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

print(f"Markdown créé avec succès: {os.path.getsize('reports/tests.md')} bytes")
PY

# Étape 6: Affichage final
print_info "6. Résultat final..."
echo ""
cat reports/tests.md

print_header "SIMULATION TERMINÉE"

if [[ -f reports/tests.xml ]]; then
    tests_count=$(python3 -c "
import xml.etree.ElementTree as ET
tree = ET.parse('reports/tests.xml')
root = tree.getroot()
print(root.get('tests', '0'))
")
    
    failures_count=$(python3 -c "
import xml.etree.ElementTree as ET
tree = ET.parse('reports/tests.xml')
root = tree.getroot()
print(root.get('failures', '0'))
")
    
    if [ "$failures_count" -eq 0 ] && [ "$tests_count" -gt 0 ]; then
        print_success "✅ $tests_count tests passés avec succès!"
    else
        print_error "❌ $failures_count test(s) échoué(s) sur $tests_count"
        exit 1
    fi
else
    print_error "❌ Aucun test exécuté"
    exit 1
fi
