#pragma once
#include "esphome/core/component.h"
#include "esphome/core/application.h"
#include <vector>
#include <map>
#include <string>
#include <list>

#ifdef IS_ROOT
#include "esphome/components/mqtt/mqtt_client.h"
#endif

namespace esphome {
namespace esp_mesh {

// Limite di sicurezza peer cifrati (Max HW è 17, teniamo margine)
#define MAX_PEERS 6 

enum PktType : uint8_t {
    PKT_PROBE   = 0x01, 
    PKT_ANNOUNCE= 0x02, 
    PKT_REG     = 0x10, 
    PKT_DATA    = 0x20, 
    PKT_CMD     = 0x30  
};

struct __attribute__((packed)) MeshHeader {
    uint8_t type;
    uint32_t net_id;
    uint8_t src[6];      // Originator
    uint8_t dst[6];      // Final Destination
    uint8_t next_hop[6]; // Immediate Receiver (Routing)
    uint8_t ttl;         // Time To Live
};

struct __attribute__((packed)) RegPayload {
    uint32_t entity_hash;
    char type_id;
    char name[24];
    char unit[8];
    char dev_class[16];
};

// Routing Entry
struct RouteInfo {
    uint8_t next_hop[6];
    uint32_t last_seen;
};

class EspMesh : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  // --- SETTERS (L'interfaccia per Python) ---
  // Python chiamerà questi metodi per passare i dati del YAML
  void set_mesh_id(const std::string &id);
  void set_pmk(const std::string &pmk);
  void set_channel(uint8_t channel); // Solo per Node
  
#ifdef IS_ROOT
  void set_mqtt(mqtt::MQTTClient *m) { mqtt_ = m; }
#endif

 protected:
  std::string pmk_;
  uint32_t net_id_hash_;
  uint8_t my_mac_[6];

  // Routing State
  uint8_t parent_mac_[6];
  uint8_t hop_count_ = 0xFF;
  std::map<std::string, RouteInfo> routes_;
  
  // Peer Management (LRU)
  std::list<std::string> peer_lru_; 

#ifdef IS_NODE
  bool scanning_ = true;
  uint8_t current_scan_ch_ = 1;
  uint32_t last_scan_step_ = 0;
  uint32_t last_announce_sent_ = 0;

  void setup_bare_metal();
  void send_probe();
  void scan_local_entities();
#endif

#ifdef IS_ROOT
  mqtt::MQTTClient *mqtt_{nullptr};
  uint32_t last_announce_ = 0;
  void handle_reg(const uint8_t *origin, const RegPayload *p);
  void handle_data(const uint8_t *origin, const uint8_t *payload);
#endif

  // Core Networking
  void on_packet(const uint8_t *mac, const uint8_t *data, int len, int8_t rssi);
  void route_packet(MeshHeader *h, const uint8_t *payload, int len);
  
  // Low Level Helpers
  void send_raw(const uint8_t *next_hop, const uint8_t *data, int len);
  void ensure_peer_slot(const uint8_t *mac);
  void derive_lmk(const uint8_t *mac, uint8_t *lmk);
  uint32_t djb2_hash(const std::string &s);
};

}
}