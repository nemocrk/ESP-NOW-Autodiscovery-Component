# 📊 Example Coverage Report

## Overview

Ecco come appare il **vero coverage report** generato dal workflow.

---

## 📊 Dashboard Principale

```
┌────────────────────────────────────────────────────────────────────────────────┐
│                     ESP-NOW Mesh - Code Coverage Report                         │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  Overall Line Coverage: 90.5%                                                   │
│                                                                                  │
│  [█████████████████████████████████████████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░]  │
│                                                                                  │
│  Function Coverage: 90.0% (18/20)                                               │
│  Branch Coverage: 88.8% (207/233)                                               │
│                                                                                  │
└────────────────────────────────────────────────────────────────────────────────┘
```

---

## 📁 File Coverage Breakdown

```
┌────────────────────────────────────────────────────────────────────────────────┐
│ Directory: components/esp_mesh/                                                 │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  File               Lines        Functions     Branches                         │
│  ────────────────  ────────────  ────────────  ────────────                  │
│                                                                                  │
│  🟢 mesh.cpp       315/350      18/20         142/160                        │
│                     (90.0%)      (90.0%)       (88.8%)                        │
│                                                                                  │
│                     [██████████████████░░]                                        │
│                                                                                  │
│  ──────────────────────────────────────────────────────────────────  │
│                                                                                  │
│  🟢 mesh_logic.h   145/150      8/8           65/68                          │
│                     (96.7%)      (100%)        (95.6%)                        │
│                                                                                  │
│                     [███████████████████░]                                        │
│                                                                                  │
│  ──────────────────────────────────────────────────────────────────  │
│                                                                                  │
│  🟡 mesh_mqtt.cpp  120/200      5/8           45/80                          │
│                     (60.0%)      (62.5%)       (56.3%)                        │
│                                                                                  │
│                     [████████████░░░░░░░░]                                        │
│                                                                                  │
└────────────────────────────────────────────────────────────────────────────────┘

Legend: 🟢 Good (>85%)  🟡 OK (70-85%)  🔴 Low (<70%)
```

---

## 🔍 Line-by-Line Coverage (mesh.cpp)

```cpp
      | // components/esp_mesh/mesh.cpp
      | #include "mesh.h"
      |
      | namespace esphome {
      | namespace esp_mesh {
      |
  118 | void EspMesh::setup() {
  118 |   ESP_LOGI(TAG, "Setting up ESP-NOW Mesh...");
      |   
  118 |   // Initialize ESP-NOW
  118 |   if (esp_now_init() != ESP_OK) {
    2 |     ESP_LOGE(TAG, "ESP-NOW init failed");
    2 |     this->mark_failed();
    2 |     return;
      |   }
      |   
  116 |   // Set PMK
  116 |   if (this->pmk_.length() == 16) {
  115 |     esp_now_set_pmk((uint8_t*)this->pmk_.c_str());
      |   } else {
    1 |     ESP_LOGW(TAG, "PMK length != 16, using default");
      |   }
      |   
  116 |   // Register callbacks
  116 |   esp_now_register_recv_cb(on_recv_cb);
  116 |   esp_now_register_send_cb(on_send_cb);
      | }
      |
      | void EspMesh::on_packet(const uint8_t *mac, const uint8_t *data, 
      |                         int len, int8_t rssi) {
  452 |   if (len < sizeof(MeshHeader)) return;  // ✅ COVERED
      |   
  450 |   auto *h = reinterpret_cast<const MeshHeader *>(data);  // ✅ COVERED
      |   
      |   // Validate packet
  450 |   if (h->net_id != this->net_id_hash_) return;  // ✅ COVERED
  445 |   if (h->ttl == 0) return;  // ✅ COVERED
      |   
      |   // Learn route
  442 |   this->learn_route(h->src, mac, millis());  // ✅ COVERED
      |   
      |   // Handle by type
  442 |   switch (h->type) {
  120 |     case PKT_PROBE:    // ✅ COVERED
  120 |       this->handle_probe(h, mac, rssi);
  120 |       break;
      |       
  110 |     case PKT_ANNOUNCE:  // ✅ COVERED
  110 |       this->handle_announce(h, mac, rssi);
  110 |       break;
      |       
   80 |     case PKT_REG:  // ✅ COVERED
   80 |       this->handle_reg(h, data + sizeof(MeshHeader), len - sizeof(MeshHeader));
   80 |       break;
      |       
   50 |     case PKT_DATA:  // ✅ COVERED
   50 |       this->handle_data(h, data + sizeof(MeshHeader), len - sizeof(MeshHeader));
   50 |       break;
      |       
    0 |     case PKT_CMD:  // ❌ NOT COVERED
    0 |       this->handle_cmd(h, data + sizeof(MeshHeader), len - sizeof(MeshHeader));
    0 |       break;
      |       
   82 |     default:  // ✅ COVERED
   82 |       ESP_LOGW(TAG, "Unknown packet type: 0x%02X", h->type);
   82 |       break;
      |   }
      | }
```

**Legend:**
- Numero a sinistra = numero di volte eseguita la riga
- ✅ Verde = Riga coperta
- ❌ Rosso = Riga NON coperta
- Vuoto = Riga non eseguibile (commenti, dichiarazioni)

---

## 🔬 Function Coverage Detail

```
┌────────────────────────────────────────────────────────────────────────────────┐
│ Function Name                      Called    Coverage                           │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ✅ setup()                        118x      100%                               │
│  ✅ on_packet()                    452x      95.0%                              │
│  ✅ handle_probe()                 120x      100%                               │
│  ✅ handle_announce()              110x      100%                               │
│  ✅ handle_reg()                   80x       100%                               │
│  ✅ handle_data()                  50x       100%                               │
│  ❌ handle_cmd()                   0x        0%    ← NOT TESTED!             │
│  ✅ learn_route()                  442x      100%                               │
│  ✅ find_route()                   150x      100%                               │
│  ✅ gc_old_routes()                25x       100%                               │
│  ✅ ensure_peer_slot()             200x      100%                               │
│  ✅ send_raw()                     180x      100%                               │
│  ✅ derive_lmk()                   200x      100%                               │
│  ✅ djb2_hash()                    10x       100%                               │
│  ✅ validate_packet_header()       452x      100%                               │
│  ✅ is_broadcast()                 60x       100%                               │
│  ✅ mac_equal()                    80x       100%                               │
│  ❌ scan_local_entities()          0x        0%    ← NOT TESTED!             │
│  ✅ add_peer()                     200x      100%                               │
│  ✅ clear_peers()                  5x        100%                               │
│                                                                                  │
│  TOTAL: 18/20 functions covered (90.0%)                                         │
│                                                                                  │
└────────────────────────────────────────────────────────────────────────────────┘
```

---

## 🌿 Branch Coverage Detail

```
Branch Coverage for on_packet():

if (len < sizeof(MeshHeader)) return;
│
├─ True:  2 executions  ✅
└─ False: 450 executions ✅

if (h->net_id != this->net_id_hash_) return;
│
├─ True:  5 executions  ✅
└─ False: 445 executions ✅

if (h->ttl == 0) return;
│
├─ True:  3 executions  ✅
└─ False: 442 executions ✅

switch (h->type)
├─ PKT_PROBE:    120 executions ✅
├─ PKT_ANNOUNCE: 110 executions ✅
├─ PKT_REG:      80 executions  ✅
├─ PKT_DATA:     50 executions  ✅
├─ PKT_CMD:      0 executions   ❌ NOT COVERED
└─ default:      82 executions  ✅

Branch Coverage: 88.8% (142/160 branches)
```

---

## 🚨 Uncovered Code Hotspots

```
┌────────────────────────────────────────────────────────────────────────────────┐
│ 🔴 TOP 5 UNCOVERED CODE SECTIONS                                                │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  1. mesh.cpp:256-270 (handle_cmd function)                                     │
│     15 lines NOT covered                                                        │
│     Reason: PKT_CMD never sent in tests                                         │
│                                                                                  │
│  2. mesh.cpp:350-365 (scan_local_entities function)                            │
│     16 lines NOT covered                                                        │
│     Reason: Requires ESPHome App context                                        │
│                                                                                  │
│  3. mesh_mqtt.cpp:50-70 (MQTT publish logic)                                   │
│     21 lines NOT covered                                                        │
│     Reason: MQTT client not mocked                                              │
│                                                                                  │
│  4. mesh.cpp:400-410 (Error recovery path)                                     │
│     11 lines NOT covered                                                        │
│     Reason: Error conditions not triggered                                      │
│                                                                                  │
│  5. mesh.cpp:450-455 (Edge case validation)                                    │
│     6 lines NOT covered                                                         │
│     Reason: Specific edge case not in test data                                 │
│                                                                                  │
└────────────────────────────────────────────────────────────────────────────────┘

Action Items:
- Add test for PKT_CMD packet type
- Mock ESPHome App for entity scanning
- Add MQTT mock for publish tests
- Add error injection tests
- Add edge case test data
```

---

## 📊 Coverage Trend

```
Last 10 Commits:

Commit    Date        Coverage   Change
───────────────────────────────────────────────

ab123f    Dec 16      90.5%      +2.5%  ⬆️
cd456e    Dec 15      88.0%      +1.0%  ⬆️
ef789a    Dec 14      87.0%      -0.5%  ⬇️
gh012b    Dec 13      87.5%      +3.0%  ⬆️
ij345c    Dec 12      84.5%      +1.5%  ⬆️
kl678d    Dec 11      83.0%      +0.5%  ⬆️
mn901e    Dec 10      82.5%      +2.0%  ⬆️
op234f    Dec 9       80.5%      +1.0%  ⬆️
qr567g    Dec 8       79.5%      +0.5%  ⬆️
st890h    Dec 7       79.0%       ---   ➡️

Trend: 📈 Increasing (+11.5% in 10 days)
```

---

## ✅ Action Items from Coverage Report

```markdown
## To Reach 95% Coverage:

### High Priority (🔴)
1. Add test for PKT_CMD handling (15 lines)
2. Mock Entity Manager for scan_local_entities() (16 lines)

### Medium Priority (🟡)
3. Add MQTT mock for publish tests (21 lines)
4. Add error injection tests (11 lines)

### Low Priority (🟢)
5. Add edge case test data (6 lines)

Total uncovered: 69 lines
Estimated effort: 4-6 hours
Target coverage after fixes: 95.5%
```

---

**Questo è il tipo di report che il workflow genera automaticamente!** 🎉
