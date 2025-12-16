# C++ Unit Tests - ESP-NOW Mesh Component

Test suite completo per il componente C++ `esp_mesh` usando **Google Test** (gtest).

## 📋 Struttura

```
tests/
├── CMakeLists.txt                    # Build configuration
├── mocks/
│   ├── mock_esp_now.h               # Mock for ESP-NOW functions
│   └── mock_wifi.h                  # Mock for WiFi functions
├── src/
│   ├── test_esp_mesh_core.cpp       # Setup, initialization, hash
│   ├── test_esp_mesh_networking.cpp # Packet handling, types
│   ├── test_esp_mesh_routing.cpp    # Routing decisions, TTL, reverse path
│   ├── test_esp_mesh_peer_mgmt.cpp  # LRU eviction, LMK derivation
│   └── test_esp_mesh_validation.cpp # Schema validation, bounds checking
└── README.md                         # This file
```

## 🏗️ Requisiti

- **CMake** >= 3.16
- **C++** compiler (GCC/Clang) con standard C++17
- **Google Test** (scaricato automaticamente via `FetchContent`)

### Ubuntu/Debian

```bash
sudo apt-get install -y cmake build-essential g++ gcov lcov
```

### macOS

```bash
brew install cmake gcc
```

## 🔧 Compilazione

### Build Locale - NODE Mode

```bash
mkdir -p build-node
cd build-node

cmake ../tests \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-DNODE_MODE=1 -Wall -Wextra"

cmake --build . --config Debug -j$(nproc)
```

### Build Locale - ROOT Mode

```bash
mkdir -p build-root
cd build-root

cmake ../tests \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-DROOT_MODE=1 -Wall -Wextra"

cmake --build . --config Debug -j$(nproc)
```

## ▶️ Esecuzione

### Run All Tests

```bash
cd build-node
./esp_mesh_tests
```

### Run Specific Test Suite

```bash
# Core setup tests only
./esp_mesh_tests --gtest_filter="*SetupTest.*"

# Networking tests
./esp_mesh_tests --gtest_filter="NetworkingTest.*"

# Routing tests
./esp_mesh_tests --gtest_filter="RoutingTest.*"

# Peer management tests
./esp_mesh_tests --gtest_filter="PeerMgmtTest.*"

# Validation tests
./esp_mesh_tests --gtest_filter="ValidationTest.*"
```

### Run Single Test

```bash
./esp_mesh_tests --gtest_filter="RoutingTest.TTLDecrementOnForward"
```

### Verbose Output

```bash
./esp_mesh_tests --gtest_filter="*" -v
```

### Generate XML Report

```bash
./esp_mesh_tests --gtest_output="xml:test-results.xml"
```

## 📊 Test Coverage

### Build with Coverage

```bash
mkdir -p build-coverage
cd build-coverage

cmake ../tests \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage"

cmake --build . --config Debug -j$(nproc)
```

### Generate Coverage Report

```bash
cd build-coverage
./esp_mesh_tests

lcov --directory . --capture --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/googletest/*' --output-file coverage.info
lcov --list coverage.info

# Generate HTML report
genhtml coverage.info --output-directory coverage_report
open coverage_report/index.html  # macOS
# xdg-open coverage_report/index.html  # Linux
```

## 🧪 Test Categories

### NODE Tests

Testi per la modalità NODE (sensore):

- **Core Setup Tests** (`EspMeshSetupTest`)
  - Inizializzazione NODE
  - PMK validation
  - Hash computation (DJB2)
  - Channel setting

- **Networking Tests** (`NetworkingTest`)
  - Packet reception & parsing
  - ANNOUNCE packet handling
  - PROBE scanning
  - PKT_DATA format validation

- **Routing Tests** (`RoutingTest`)
  - Reverse path learning
  - TTL handling & decrement
  - Multi-hop routing
  - Loop prevention

### ROOT Tests

Testi per la modalità ROOT (gateway):

- **Peer Management Tests** (`PeerMgmtTest`)
  - Aggiunta peer
  - LRU eviction quando tabella piena
  - Protezione parent da eviction
  - LMK derivation deterministico

- **Validation Tests** (`ValidationTest`)
  - PMK length (exactly 16 chars)
  - Mode enum validation
  - Channel range (1-13)
  - Hash determinism

## 🔗 GitHub Actions

I test vengono eseguiti automaticamente su:

- **Push** a `main`, `feature/test-suite`, `feature/*`
- **Pull Requests** verso `main`
- **Trigger manuale** (workflow dispatch)

### Workflow: `.github/workflows/cpp-unit-tests.yml`

Jobs:

1. **unit-tests-node**: NODE mode tests (core, networking, routing)
2. **unit-tests-root**: ROOT mode tests (peer mgmt, validation)
3. **code-coverage**: Code coverage analysis & upload to Codecov
4. **test-summary**: Test results summary in PR comments

## 📝 Scrivere Nuovi Test

Template per un nuovo test:

```cpp
#include <gtest/gtest.h>

class MyTestSuite : public ::testing::Test {
public:
    void SetUp() override {
        // Setup before each test
    }

    void TearDown() override {
        // Cleanup after each test
    }
};

/**
 * TEST: DescriptionHere
 * 
 * Verifica che...
 * 
 * Prerequisiti:
 *   - ...
 * 
 * Azioni:
 *   1. ...
 *   2. ...
 * 
 * Risultato Atteso:
 *   - ...
 */
TEST_F(MyTestSuite, TestName) {
    // Arrange
    int expected = 42;
    
    // Act
    int result = some_function();
    
    // Assert
    EXPECT_EQ(result, expected);
}
```

### Common Assertions

```cpp
EXPECT_EQ(a, b);        // a == b
EXPECT_NE(a, b);        // a != b
EXPECT_LT(a, b);        // a < b
EXPECT_LE(a, b);        // a <= b
EXPECT_GT(a, b);        // a > b
EXPECT_GE(a, b);        // a >= b
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);
```

## 🎯 Best Practices

1. **Arrange-Act-Assert**: Struttura chiara dei test
2. **One assertion per concetto**: Non mischiare logica di test
3. **Nomi descrittivi**: `TestName` deve spiegare cosa viene testato
4. **Commenti dettagliati**: Documentare prerequisiti e risultati attesi
5. **Mock appropriati**: Usare mock per componenti esterni (ESP-NOW, WiFi)
6. **Test deterministici**: Evitare dipendenze da tempo o stato globale

## 🚀 Continuous Integration

I test vengono eseguiti:

- ✅ Su ogni push al branch
- ✅ Su ogni pull request
- ✅ Con coverage report
- ✅ Con risultati pubblicati in PR comments

## 📚 Risorse

- [Google Test Documentation](https://github.com/google/googletest/blob/main/docs/primer.md)
- [CMake Documentation](https://cmake.org/cmake/help/latest/)
- [C++ Reference](https://en.cppreference.com/)

## ❓ Troubleshooting

### CMake not found

```bash
sudo apt-get install cmake
# or
brew install cmake
```

### G++ too old (need C++17)

```bash
sudo apt-get install g++-11
export CXX=g++-11
export CC=gcc-11
```

### gtest_discover_tests not working

```bash
# Update CMake
sudo apt-get install --only-upgrade cmake
```

### Coverage info empty

```bash
# Make sure -fprofile-arcs and -ftest-coverage are set
# Run tests after build
```

---

**Ultima modifica**: December 2025
**Versione**: 1.0
