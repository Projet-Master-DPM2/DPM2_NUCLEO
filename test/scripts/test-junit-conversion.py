#!/usr/bin/env python3

"""
Script pour tester la conversion des résultats Unity vers JUnit XML
Usage: python3 test-junit-conversion.py
"""

import re
import xml.etree.ElementTree as ET
from datetime import datetime
import subprocess
import os

def run_tests_and_convert():
    """Exécuter les tests et convertir vers JUnit XML"""
    
    print("=== Test de conversion Unity → JUnit XML ===")
    
    # Exécuter les tests
    print("1. Exécution des tests...")
    try:
        result = subprocess.run(['make', 'test-native'], 
                              capture_output=True, text=True, cwd='.')
        content = result.stdout + result.stderr
        
        # Sauvegarder dans un fichier pour debug
        with open('reports/tests_raw.log', 'w') as f:
            f.write(content)
            
        print(f"Tests exécutés (code retour: {result.returncode})")
        
    except Exception as e:
        print(f"Erreur lors de l'exécution des tests: {e}")
        return False
    
    print("\n2. Analyse du contenu...")
    print("Premiers 500 caractères:")
    print(repr(content[:500]))
    
    # Parser les résultats Unity - pattern correct observé
    unity_pattern = r'\./(.*?):(\d+):(.*?):(PASS|FAIL)'
    unity_matches = re.findall(unity_pattern, content)
    
    # Parser les résultats de résumé Makefile
    makefile_success_pattern = r'✅ (.*?): SUCCÈS'
    makefile_fail_pattern = r'❌ (.*?): ÉCHEC'
    
    success_tests = re.findall(makefile_success_pattern, content)
    failed_tests = re.findall(makefile_fail_pattern, content)
    
    print(f"\n3. Résultats du parsing:")
    print(f"Unity matches: {len(unity_matches)}")
    print(f"Makefile success: {len(success_tests)}")
    print(f"Makefile failed: {len(failed_tests)}")
    
    # Si on a des résultats Unity détaillés, les utiliser
    if unity_matches:
        matches = unity_matches
        print("Utilisation des résultats Unity détaillés")
        for match in unity_matches[:5]:
            print(f"  Unity: {match}")
    else:
        # Sinon, créer des matches basés sur les résultats Makefile
        matches = []
        for test_name in success_tests:
            matches.append((f"test/{test_name}", "1", test_name, "PASS"))
        for test_name in failed_tests:
            matches.append((f"test/{test_name}", "1", test_name, "FAIL"))
        print("Utilisation des résultats Makefile")
        for match in matches[:5]:
            print(f"  Makefile: {match}")
    
    print(f"\nTotal matches: {len(matches)}")
    
    if not matches:
        print("❌ Aucun test trouvé!")
        return False
    
    print("\n4. Génération XML JUnit...")
    
    # Créer XML JUnit
    root = ET.Element('testsuites')
    root.set('name', 'NUCLEO Native Tests')
    root.set('timestamp', datetime.now().isoformat())
    
    # Grouper par fichier de test
    test_files = {}
    for match in matches:
        if len(match) == 4:  # Unity format
            file_path, line_num, test_name, status = match
        else:  # Makefile format
            file_path, line_num, test_name, status = match
            
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
    os.makedirs('reports', exist_ok=True)
    tree = ET.ElementTree(root)
    tree.write('reports/tests.xml', encoding='utf-8', xml_declaration=True)
    
    print(f"✅ JUnit XML créé avec {total_tests} tests, {total_failures} échecs")
    
    # Vérifier le contenu du XML
    with open('reports/tests.xml', 'r') as f:
        xml_content = f.read()
    
    print(f"\n5. Contenu XML généré ({len(xml_content)} caractères):")
    print(xml_content[:500] + "..." if len(xml_content) > 500 else xml_content)
    
    return total_tests > 0

if __name__ == "__main__":
    success = run_tests_and_convert()
    exit(0 if success else 1)
