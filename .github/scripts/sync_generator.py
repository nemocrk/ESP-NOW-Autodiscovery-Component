import os
import re

# CONFIGURAZIONE
# Mappa il nome della funzione JS al file sorgente reale
FILE_MAPPING = {
    'genCpp': 'components/esp_mesh/mesh.cpp',
    'genH':   'components/esp_mesh/mesh.h',
    'genPy':  'components/esp_mesh/__init__.py',
    # Nota per il YAML: La funzione genYaml contiene logica IF/ELSE in JS.
    # Se vuoi sincronizzare anche questo, il file sorgente deve contenere 
    # l'intera logica JS, oppure devi mantenere genYaml manuale nell'HTML.
    # Per ora lo lascio commentato per sicurezza.
    # 'genYaml': 'templates/config.yaml.js_template' 
}

HTML_FILE = 'docs/index.html'

def update_html():
    if not os.path.exists(HTML_FILE):
        print(f"Errore: {HTML_FILE} non trovato!")
        exit(1)

    with open(HTML_FILE, 'r', encoding='utf-8') as f:
        html_content = f.read()

    files_updated = 0

    for func_name, source_path in FILE_MAPPING.items():
        if not os.path.exists(source_path):
            print(f"Warning: File sorgente {source_path} non trovato. Salto.")
            continue

        print(f"Leggendo {source_path}...")
        with open(source_path, 'r', encoding='utf-8') as f:
            new_code = f.read()

        # Escape dei backtick se presenti nel codice C++ (raro ma possibile)
        # per non rompere il template literal JS
        new_code = new_code.replace('`', '\\`')

        # REGEX SPIEGAZIONE:
        # 1. (function\s+NOME\(c\)\s*\{\s*return\s*`) -> Trova l'inizio della funzione fino al primo backtick
        # 2. ([\s\S]*?) -> Cattura tutto il contenuto attuale (lazy match)
        # 3. (`;\s*\}) -> Trova il backtick di chiusura e la graffa fine funzione
        pattern = r'(function\s+' + func_name + r'\(c\)\s*\{\s*return\s*`)([\s\S]*?)(`;\s*\})'

        if re.search(pattern, html_content):
            # Sostituzione usando una lambda per mantenere i gruppi 1 e 3 (intestazione e chiusura) invariati
            html_content = re.sub(pattern, lambda m: m.group(1) + new_code + m.group(3), html_content)
            print(f"✅ Aggiornata funzione {func_name} con contenuto di {source_path}")
            files_updated += 1
        else:
            print(f"❌ Errore: Non ho trovato la funzione {func_name} in {HTML_FILE}")

    if files_updated > 0:
        with open(HTML_FILE, 'w', encoding='utf-8') as f:
            f.write(html_content)
        print(f"Successo! {HTML_FILE} aggiornato.")
    else:
        print("Nessuna modifica apportata.")

if __name__ == "__main__":
    update_html()