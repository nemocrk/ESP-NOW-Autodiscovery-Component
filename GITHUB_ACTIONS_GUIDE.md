# 🚀 GitHub Actions Workflow Guide

## Overview

Questo workflow completo esegue automaticamente:
- ✅ Unit Tests (mesh_logic.h)
- ✅ Integration Tests (mesh.cpp reale)
- ✅ Code Coverage Report con grafici
- ✅ Mutation Testing
- ✅ ESP32 Firmware Build
- ✅ Test Summary su PR

---

## 📦 Cosa Viene Generato

### 1. Coverage Report HTML

```
coverage_html/
├── index.html          ← Report principale con grafici
├── mesh.cpp.gcov.html  ← Coverage line-by-line
└── mesh_logic.h.gcov.html
```

**Esempio Output:**
```
Overall Coverage: 90.5%

File                   Lines    Functions    Branches
─────────────────────────────────────────────────────
mesh.cpp               315/350  18/20        142/160
mesh_logic.h           145/150  8/8          65/68
─────────────────────────────────────────────────────
TOTAL                  460/500  26/28        207/228
```

### 2. Coverage Badge

![Coverage](https://img.shields.io/badge/Coverage-90.5%25-brightgreen)

Si aggiorna automaticamente ad ogni push!

### 3. PR Comment Automatico

Su ogni Pull Request:

```markdown
## 🧪 Test Results

✅ All tests passed!

### Coverage Report

| File | Lines | Functions | Branches |
|------|-------|-----------|----------|
| mesh.cpp | 90.0% ⬆️ | 90.0% | 88.8% |
| mesh_logic.h | 96.7% | 100% | 95.6% |

### Changes
- ⬆️ mesh.cpp: +2.5% lines
- ➡️ mesh_logic.h: no change
```

---

## 🔧 Setup

### Step 1: Abilita GitHub Actions

Il workflow è già in `.github/workflows/test-and-coverage.yml`.

Push al branch e GitHub Actions parte automaticamente!

### Step 2: Setup Secrets (Opzionali)

#### Per Coverage Badge:

1. Crea un Gist:
   - Vai su https://gist.github.com
   - Crea gist pubblico `esp-mesh-coverage.json`
   - Content: `{"schemaVersion": 1}`

2. Crea Personal Access Token:
   - Settings → Developer settings → Personal access tokens
   - Scope: `gist`

3. Aggiungi Secret al repository:
   - Settings → Secrets → New repository secret
   - Name: `GIST_SECRET`
   - Value: `<your_token>`

4. Update workflow:
   ```yaml
   gistID: <your_gist_id>  # Da URL del gist
   ```

#### Per Codecov (Opzionale):

1. Vai su https://codecov.io
2. Connetti il repository
3. Copia il token
4. Aggiungi secret `CODECOV_TOKEN`

---

## 🎯 Workflow Jobs

### Job 1: Unit Tests

```yaml
unit-tests:
  runs-on: ubuntu-latest
  steps:
    - Checkout code
    - Install PlatformIO
    - Run: pio test -e native_test
    - Upload results
```

**Tempo:** ~30 secondi

### Job 2: Integration Tests + Coverage

```yaml
integration-tests:
  runs-on: ubuntu-latest
  steps:
    - Checkout code
    - Install PlatformIO + lcov
    - Run: pio test -e native_test_coverage
    - Generate HTML coverage report
    - Generate badge
    - Upload to Codecov
    - Comment on PR
```

**Tempo:** ~1 minuto

**Output:**
- `coverage_html/` (artifacts)
- Coverage badge aggiornato
- Comment su PR

### Job 3: Mutation Testing

```yaml
mutation-tests:
  runs-on: ubuntu-latest
  steps:
    - Install Cosmic Ray
    - Run 20 mutations
    - Generate report
```

**Tempo:** ~2 minuti

**Output:**
```
Mutation Score: 80%
8/10 mutants killed

Surviving mutants:
- mesh.cpp:125 - Comment change (non-critical)
- mesh.cpp:200 - Format string change (non-critical)
```

### Job 4: ESP32 Build

```yaml
esp32-build:
  runs-on: ubuntu-latest
  steps:
    - Checkout code
    - Install PlatformIO
    - Run: pio run -e esp32
    - Upload firmware.bin
```

**Tempo:** ~1 minuto

**Output:** `firmware.bin` (artifacts)

### Job 5: Test Summary

```yaml
test-summary:
  runs-on: ubuntu-latest
  needs: [unit-tests, integration-tests, esp32-build]
  steps:
    - Download all artifacts
    - Generate summary markdown
    - Comment on PR
```

**Tempo:** ~10 secondi

---

## 📊 Coverage Report Visuale

### Come Visualizzare Localmente

```bash
# 1. Esegui test con coverage
pio test -e native_test_coverage

# 2. Genera report HTML
lcov --capture \
     --directory .pio/build/native_test_coverage \
     --output-file coverage.info

lcov --remove coverage.info '/usr/*' '*/test/*' --output-file coverage_filtered.info

genhtml coverage_filtered.info --output-directory coverage_html

# 3. Apri nel browser
open coverage_html/index.html  # macOS
xdg-open coverage_html/index.html  # Linux
```

### Report Features

**Index Page:**
```
┌─────────────────────────────────────────────┐
│  ESP-NOW Mesh Coverage Report              │
├─────────────────────────────────────────────┤
│  Overall Coverage: 90.5%                    │
│                                             │
│  [████████████████████░░] 90.5%            │
│                                             │
│  Files:                                     │
│  • components/esp_mesh/mesh.cpp      90.0% │
│  • components/esp_mesh/mesh_logic.h  96.7% │
└─────────────────────────────────────────────┘
```

**File Detail (mesh.cpp):**
```html
120: ✅ void EspMesh::on_packet(...) {
121: ✅   if (len < sizeof(MeshHeader)) return;
122: ✅   auto *h = reinterpret_cast<const MeshHeader *>(data);
123: ✅   if (h->net_id != this->net_id_hash_) return;  // Tested!
124: ❌   if (h->magic == 0xDEAD) return;  // NOT TESTED
125: ✅   this->learn_route(h->src, mac, millis());
```

- ✅ Verde = Linea coperta
- ❌ Rosso = Linea NON coperta
- 🟡 Giallo = Branch parzialmente coperto

---

## 🎨 Badge nel README

Aggiungi al tuo `README.md`:

```markdown
# ESP-NOW Autodiscovery Component

![Tests](https://github.com/nemocrk/ESP-NOW-Autodiscovery-Component/workflows/Tests%20%26%20Coverage/badge.svg)
![Coverage](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/<username>/<gist_id>/raw/esp-mesh-coverage.json)
![Build](https://img.shields.io/badge/build-passing-brightgreen)
![License](https://img.shields.io/badge/license-MIT-blue)
```

**Risultato:**

![Tests](https://github.com/nemocrk/ESP-NOW-Autodiscovery-Component/workflows/Tests%20%26%20Coverage/badge.svg)
![Coverage](https://img.shields.io/badge/Coverage-90.5%25-brightgreen)
![Build](https://img.shields.io/badge/build-passing-brightgreen)

---

## 🔍 PR Review Flow

### Quando Fai un PR:

**1. GitHub Actions Runs Automatically**

```
✅ Unit Tests        (30s)
✅ Integration Tests (1m)
✅ Coverage Report   (1m)
✅ Mutation Tests    (2m)
✅ ESP32 Build       (1m)
```

**2. Bot Commenta sul PR:**

```markdown
## 🧪 Test Results for PR #42

### ✅ All Tests Passed!

**Test Summary:**
- Unit Tests: 15/15 passed ✅
- Integration Tests: 25/25 passed ✅
- ESP32 Build: Success ✅

**Coverage Report:**

| Metric | Before | After | Change |
|--------|--------|-------|--------|
| Lines  | 88.0%  | 90.5% | +2.5% ⬆️ |
| Functions | 88.0% | 90.0% | +2.0% ⬆️ |
| Branches | 86.0% | 88.8% | +2.8% ⬆️ |

**Detailed Coverage:**
- mesh.cpp: 90.0% (+2.5%)
- mesh_logic.h: 96.7% (no change)

**Mutation Testing:**
- Mutation Score: 80%
- 8/10 mutants killed

**Download Artifacts:**
- [Coverage HTML Report](link)
- [ESP32 Firmware](link)
- [Mutation Report](link)

---
✅ Ready to merge!
```

**3. Reviewer Può:**
- Vedere coverage direttamente nel PR
- Download HTML report per dettagli
- Verificare che nessun bug sia sfuggito

---

## 🐛 Coverage Decrease Protection

Il workflow **FALLISCE** se:

```yaml
minimum_coverage: 75  # Minimo 75%
```

Se coverage scende sotto 75%, il PR è bloccato:

```
❌ Coverage check failed!

Current coverage: 72.5%
Minimum required: 75.0%

Please add tests to cover:
- mesh.cpp:124-130 (not covered)
- mesh.cpp:200-205 (not covered)
```

---

## 📈 Coverage Trend nel Tempo

Codecov genera grafici automatici:

```
Coverage Trend (Last 30 Days)

100% ┤
 90% ┤        ╭─────────
 80% ┤   ╭────╯
 70% ┤───╯
     └─────────────────────────────>
     Oct  Nov  Dec
```

**Insights:**
- 📈 Coverage in crescita: +15% in 2 mesi
- 🎯 Target 95% entro gennaio

---

## 🎯 Obiettivi di Coverage

### Current State
```
✅ mesh_logic.h:  96.7% (OTTIMO)
✅ mesh.cpp:      90.0% (BUONO)
⚠️ mesh_mqtt.cpp: 60.0% (DA MIGLIORARE)
```

### Target Q1 2026
```
🎯 mesh_logic.h:  98%+
🎯 mesh.cpp:      95%+
🎯 mesh_mqtt.cpp: 85%+
🎯 OVERALL:       93%+
```

---

## 🚨 Troubleshooting

### Workflow Fallisce su Coverage Generation

```bash
# Verifica localmente
pio test -e native_test_coverage -vv

# Check se .gcda files sono generati
find .pio -name "*.gcda"
```

### Coverage Badge Non Si Aggiorna

1. Verifica Gist ID nel workflow
2. Verifica GIST_SECRET secret
3. Check permissions del token (scope: gist)

### Mutation Tests Timeout

```yaml
# Riduci numero di mutations
cosmic-ray exec --max-mutations 10 mutation-session
```

---

## 📚 Artifacts

Ogni run genera:

```
Artifacts/
├── coverage-report/
│   └── index.html         (Coverage HTML)
├── unit-test-results/
│   └── test_output.txt    (Test logs)
├── mutation-report/
│   └── mutation_report.txt (Mutation results)
├── esp32-firmware/
│   └── firmware.bin       (ESP32 binary)
└── test-summary/
    └── test_summary.md    (Summary)
```

**Retention:** 90 giorni

---

## 🎉 Conclusione

Con questo workflow:

✅ **Ogni commit è testato automaticamente**  
✅ **Coverage visualizzato in tempo reale**  
✅ **PR review ha tutti i dati necessari**  
✅ **Bug rilevati prima del merge**  
✅ **Firmware ESP32 sempre compilabile**  

**Zero sforzo manuale. Tutto automatico.** 🚀
