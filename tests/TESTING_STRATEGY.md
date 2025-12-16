# Testing Strategy - ESP-NOW Mesh Component

## 🎯 Obiettivo

Test suite **unit test C++ reali** che testano il codice C++ del componente `esp_mesh`, non mock Python inutili.

## ❌ Cosa NON stiamo facendo

- ❌ Test Python puri che non testano niente
- ❌ Mock su mock senza collegamento al codice reale
- ❌ Test senza logica di verifica effettiva
- ❌ Fixture che non replicano il comportamento hardware

## ✅ Cosa stiamo facendo

### 1. **C++ Unit Tests con Google Test**

- **Framework**: Google Test (gtest) - standard industry
- **Compilazione**: CMake con C++17
- **Esecuzione**: Locale e in GitHub Actions
- **Coverage**: LCOV per analisi code coverage

### 2. **Struttura dei Test**

#### **NODE Tests** (sensori mesh)

Test suite incentrati su modalità NODE:

```
tests/src/test_esp_mesh_core.cpp
├── EspMeshSetupTest
│   ├── NodeSetupInitialization      // Setup NODE
│   ├── PMKSetterValidation          // Validazione PMK (16 chars)
│   ├── MeshIdHashComputation        // DJB2 hash
│   └── ChannelSetterNode            // Impostazione canale
│
tests/src/test_esp_mesh_networking.cpp
├── NetworkingTest
│   ├── PacketReceptionValidMeshHeader   // Parsing header
│   ├── AnnouncePacketNodeFirstParent    // Primo ANNOUNCE
│   ├── ProbeChannelScanCycle            // Scansione canali 1-13
│   ├── PacketDataFormatSensor           // Payload 8 bytes (float)
│   ├── PacketDataFormatBinarySensor     // Payload 5 bytes (bool)
│   └── BroadcastDestinationIdentification
│
tests/src/test_esp_mesh_routing.cpp
├── RoutingTest
│   ├── ReversePathLearningFromSource    // Learning MAC
│   ├── TTLDecrementOnForward            // TTL--
│   ├── TTLZeroNotForwarded              // Early return
│   ├── ForwardPreservesSource            // Src invariant
│   ├── MultiHopPath                      // 3+ hops
│   └── RoutingLoopPrevention
```

#### **ROOT Tests** (gateway)

Test suite incentrati su modalità ROOT:

```
tests/src/test_esp_mesh_peer_mgmt.cpp
├── PeerMgmtTest
│   ├── AddNewPeerSuccess               // Aggiunta peer
│   ├── LRUEvictionWhenTableFull        // Eviction LRU quando MAX_PEERS
│   ├── LRUOrderMaintained              // Order [A,B,C] -> [B,C,A]
│   ├── DeriveLMKDeterministic          // LMK = PMK XOR MAC
│   └── DeriveLMKDifferentForDifferentMACs
│
tests/src/test_esp_mesh_validation.cpp
├── ValidationTest
│   ├── PMKExactly16Chars               // Length check
│   ├── PMKTooShortError
│   ├── PMKTooLongError
│   ├── ModeNodeValid                   // MODE enum
│   ├── ModeRootValid
│   ├── ChannelRangeValid               // 1-13
│   └── MeshIdHashDeterministic
```

### 3. **Mock Strategy**

#### `tests/mocks/mock_esp_now.h`

Mock delle funzioni ESP-NOW basse livello:
- `esp_now_init()`
- `esp_now_send(mac, data, len)`
- `esp_now_add_peer(mac, encrypt, lmk)`
- `esp_now_del_peer(mac)`
- `esp_now_is_peer_exist(mac)`
- `esp_now_register_recv_cb(callback)`

#### `tests/mocks/mock_wifi.h`

Mock delle funzioni WiFi:
- `esp_wifi_init()`
- `esp_wifi_get_mac(role, mac_addr)`
- `esp_wifi_set_channel(primary, type)`

### 4. **Compilazione Separata: NODE vs ROOT**

**NODE Mode** (preprocessor define: `-DNODE_MODE=1`):
```bash
cmake ../tests -DCMAKE_CXX_FLAGS="-DNODE_MODE=1"
make
./esp_mesh_tests --gtest_filter="*NodeSetupInitialization*"
```

**ROOT Mode** (preprocessor define: `-DROOT_MODE=1`):
```bash
cmake ../tests -DCMAKE_CXX_FLAGS="-DROOT_MODE=1"
make
./esp_mesh_tests --gtest_filter="*LRUEviction*"
```

Questo permette di testare la logica differente tra NODE e ROOT con compilazioni separate.

### 5. **GitHub Actions Workflow**

File: `.github/workflows/cpp-unit-tests.yml`

**Jobs in parallelo**:

1. **unit-tests-node**
   - Build con `-DNODE_MODE=1`
   - Esegue test setup, networking, routing
   - Upload XML results

2. **unit-tests-root**
   - Build con `-DROOT_MODE=1`
   - Esegue test peer management, validation
   - Upload XML results

3. **code-coverage**
   - Build con `--coverage`
   - Esegue tutti i test
   - LCOV report → Codecov

4. **test-summary**
   - Pubblica risultati in PR comments
   - Summary report in GitHub Summary

## 📊 Coverage Goals

| Componente | Target | Metodo |
|-----------|--------|--------|
| esp_mesh_core | 90% | Unit tests + Integration |
| esp_mesh_networking | 85% | Unit tests (packet parsing) |
| esp_mesh_routing | 90% | Unit tests (TTL, path learning) |
| esp_mesh_peer_mgmt | 88% | Unit tests (LRU, LMK) |
| esp_mesh_validation | 95% | Unit tests (bounds checks) |

## 🔄 Flusso di Test

```
code change
    ↓
push to feature/* or main
    ↓
GitHub Actions triggered
    ↓
┌─────────────────┬──────────────────┐
│ NODE Tests      │ ROOT Tests       │
│ (parallel)      │ (parallel)       │
└─────────────────┴──────────────────┘
    ↓                   ↓
  XML results       XML results
    ↓                   ↓
  ┌──────────────────────┐
  │ Test Summary         │
  │ Publish to PR        │
  │ Coverage to Codecov  │
  └──────────────────────┘
    ↓
 ✅ or ❌
```

## 🚀 Esecuzione Locale

### Quick Start (NODE)

```bash
mkdir -p build-node && cd build-node
cmake ../tests -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-DNODE_MODE=1"
make
./esp_mesh_tests
```

### Con Coverage

```bash
mkdir -p build-coverage && cd build-coverage
cmake ../tests -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage"
make
./esp_mesh_tests
lcov --directory . --capture --output-file coverage.info
genhtml coverage.info -o coverage_report
```

## 📝 Aggiungere Nuovi Test

1. **Scegli il file corretto**:
   - Setup/Core → `test_esp_mesh_core.cpp`
   - Packet handling → `test_esp_mesh_networking.cpp`
   - Routing → `test_esp_mesh_routing.cpp`
   - Peer mgmt → `test_esp_mesh_peer_mgmt.cpp`
   - Schema → `test_esp_mesh_validation.cpp`

2. **Scrivi il test**:
   ```cpp
   TEST_F(MyTestSuite, TestDescription) {
       // Arrange
       auto input = "test_value";
       
       // Act
       auto result = function_under_test(input);
       
       // Assert
       EXPECT_EQ(result, "expected_output");
   }
   ```

3. **Compila e verifica**:
   ```bash
   cmake --build .
   ./esp_mesh_tests --gtest_filter="MyTestSuite.TestDescription"
   ```

## 🎓 Best Practices Implemented

✅ **Arrange-Act-Assert**: Struttura chiara  
✅ **One assertion per concetto**: Non overlapping  
✅ **Nomi descrittivi**: `TTLDecrementOnForward` vs `test1`  
✅ **Commenti doc**: Prerequisiti e risultati attesi  
✅ **Mock appropriati**: Componenti esterni isolati  
✅ **Test deterministici**: No timing deps  
✅ **Coverage tracking**: LCOV + Codecov  
✅ **CI/CD integrato**: GitHub Actions  
✅ **Separazione NODE/ROOT**: Build configs diverse  
✅ **Reportistica**: XML output + PR comments  

## 📚 Risorse

- [Google Test Primer](https://github.com/google/googletest/blob/main/docs/primer.md)
- [Effective C++ Testing](https://abseil.io/docs/cpp/)
- [CMake Best Practices](https://cliutils.gitlab.io/modern-cmake/)

---

**Status**: ✅ Production Ready  
**Last Updated**: December 2025  
**Maintainer**: nemocrk  
