# ⚡ Quick Start - Unit Tests

## 30 Secondi per Far Partire i Test

### 1. Installa PlatformIO
```bash
pip install platformio
```

### 2. Esegui i Test
```bash
pio test -e native_test
```

### 3. Output Atteso ✅
```
========== [test_mesh_logic.cpp] =========
Running 28 tests...

[ OK ] MeshLogicTest.SetMeshID
[ OK ] MeshLogicTest.SetPMKValid
[ OK ] MeshLogicTest.DJB2HashDeterministic
[ OK ] MeshLogicTest.DeriveLMKDeterministic
[ OK ] MeshLogicTest.LearnRoute
[ OK ] MeshLogicTest.AddPeer
...

========== 28 PASSED =========
```

---

## Cosa Viene Testato?

✅ **Hashing** - DJB2 deterministica e senza collisioni  
✅ **Crittografia** - LMK derivation (XOR PMK + MAC)  
✅ **Validazione** - Packet header, size, broadcast  
✅ **Routing** - Learn routes, garbage collection  
✅ **Peer Management** - LRU cache, eviction  
✅ **Strutture** - MeshHeader (24 bytes), RegPayload (53 bytes)  

---

## Comandi Utili

```bash
# Test rapido
pio test -e native_test

# Con verbose output
pio test -e native_test -v

# Solo test di hashing
pio test -e native_test --filter "*Hash*"

# Solo test di routing
pio test -e native_test --filter "*Route*"
```

---

## Struttura File

```
components/esp_mesh/
└── mesh_logic.h          ← Pure logic (testata)

test/
└── test_mesh_logic.cpp   ← Google Test suite

platformio.ini            ← Configurazione PlatformIO
```

---

**Pronto! I test dovrebbero passare tutti. 🚀**

Per dettagli completi: vedi `TEST_GUIDE.md`
