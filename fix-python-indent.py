#!/usr/bin/env python3

import re

# Lire le fichier workflow
with open('.github/workflows/nucleo-ci.yml', 'r') as f:
    content = f.read()

# Corriger l'indentation de tout le premier script Python
lines = content.split('\n')
fixed_lines = []

in_python_script = False
script_start_line = 0

for i, line in enumerate(lines):
    line_num = i + 1
    
    # Détecter le début du script Python
    if "python3 - <<'EOF'" in line:
        in_python_script = True
        script_start_line = line_num
        fixed_lines.append(line)
        continue
    
    # Détecter la fin du script Python
    if line.strip() == 'EOF' and in_python_script:
        in_python_script = False
        fixed_lines.append('            EOF')
        continue
    
    # Corriger l'indentation dans le script Python
    if in_python_script and line_num > script_start_line:
        # Supprimer l'indentation existante
        stripped = line.lstrip()
        if not stripped:  # Ligne vide
            fixed_lines.append('')
        else:
            # Calculer l'indentation Python nécessaire
            python_indent = 0
            
            # Règles d'indentation Python
            if any(stripped.startswith(keyword) for keyword in ['import ', 'from ', 'print(', '# ']):
                python_indent = 0
            elif any(stripped.startswith(keyword) for keyword in ['with ', 'if ', 'else:', 'for ', 'try:']):
                python_indent = 0
            elif any(stripped.startswith(keyword) for keyword in ['content =', 'unity_pattern =', 'unity_matches =', 'success_pattern =', 'success_tests =', 'failed_pattern =', 'failed_tests =', 'matches =', 'root =', 'total_tests =', 'total_failures =', 'tree =']):
                # Variables au niveau racine ou dans un bloc if/else
                if any(prev_line.strip().startswith(keyword) for prev_line in lines[max(0, i-3):i] for keyword in ['if ', 'else:']):
                    python_indent = 4
                else:
                    python_indent = 0
            elif stripped.startswith('matches.append(') or stripped.startswith('print('):
                # Dans une boucle for ou un bloc if
                python_indent = 4
                # Vérifier si on est dans une boucle for imbriquée
                if any(prev_line.strip().startswith('for ') for prev_line in lines[max(0, i-2):i]):
                    python_indent = 8
            else:
                # Par défaut, essayer de deviner l'indentation
                python_indent = 0
                
            # Ajouter l'indentation YAML (12 espaces) + indentation Python
            total_indent = 12 + python_indent
            fixed_lines.append(' ' * total_indent + stripped)
    else:
        fixed_lines.append(line)

# Réécrire le fichier
with open('.github/workflows/nucleo-ci.yml', 'w') as f:
    f.write('\n'.join(fixed_lines))

print("Indentation Python du workflow corrigée")

