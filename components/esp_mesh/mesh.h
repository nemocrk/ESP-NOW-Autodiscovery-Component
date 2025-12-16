#pragma once
#include "esphome/core/component.h"
#include "esphome/core/application.h"
#include "esphome/core/entity_base.h"
#include <vector>
#include <map>
#include <string>
#include <list>

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_FAN
#include "esphome/components/fan/fan.h"
#endif
#ifdef USE_CLIMATE
#include "esphome/components/climate/climate.h"
#endif
#ifdef USE_LIGHT
#include "esphome/components/light/light_state.h"
#endif
#ifdef USE_COVER
#include "esphome/components/cover/cover.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_DATETIME_DATE
#include "esphome/components/datetime/date_entity.h"
#endif
#ifdef USE_DATETIME_TIME
#include "esphome/components/datetime/time_entity.h"
#endif
#ifdef USE_DATETIME_DATETIME
#include "esphome/components/datetime/datetime_entity.h"
#endif
#ifdef USE_TEXT
#include "esphome/components/text/text.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#ifdef USE_LOCK
#include "esphome/components/lock/lock.h"
#endif
#ifdef USE_VALVE
#include "esphome/components/valve/valve.h"
#endif
#ifdef USE_MEDIA_PLAYER
#include "esphome/components/media_player/media_player.h"
#endif
#ifdef USE_ALARM_CONTROL_PANEL
#include "esphome/components/alarm_control_panel/alarm_control_panel.h"
#endif
#ifdef USE_EVENT
#include "esphome/components/event/event.h"
#endif
#ifdef USE_UPDATE
#include "esphome/components/update/update_entity.h"
#endif

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

enum EntityType : uint8_t { 
    ENTITY_TYPE_UPDATE    = 0x00,
    ENTITY_TYPE_BINARY_SENSOR   = 0x01, 
    ENTITY_TYPE_SWITCH   = 0x02, 
    ENTITY_TYPE_BUTTON   = 0x03, 
    ENTITY_TYPE_EVENT   = 0x04, 
    ENTITY_TYPE_SENSOR   = 0x05, 
    ENTITY_TYPE_TEXT_SENSOR   = 0x06, 
    ENTITY_TYPE_FAN   = 0x07, 
    ENTITY_TYPE_COVER   = 0x08, 
    ENTITY_TYPE_CLIMATE   = 0x09, 
    ENTITY_TYPE_LIGHT   = 0x0A, 
    ENTITY_TYPE_NUMBER   = 0x0B, 
    ENTITY_TYPE_DATETIME_DATE   = 0x0C, 
    ENTITY_TYPE_DATETIME_TIME   = 0x0D, 
    ENTITY_TYPE_DATETIME_DATETIME   = 0x0E, 
    ENTITY_TYPE_SELECT   = 0x0F, 
    ENTITY_TYPE_TEXT= 0x10, 
    ENTITY_TYPE_LOCK     = 0x11, 
    ENTITY_TYPE_VALVE    = 0x12, 
    ENTITY_TYPE_MEDIA_PLAYER    = 0x13, 
    ENTITY_TYPE_ALARM_CONTROL_PANEL    = 0x14
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

// Device Component
struct __attribute__((packed)) EntityInfo {
    EntityBase *entity;
    EntityType type;
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
  std::vector<EntityInfo> local_entities_{};

  void setup_bare_metal();
  void send_probe();
  void scan_local_entities();
  std::vector<EntityInfo> get_local_entities();    
  template<typename T>
    void add_entities_to_local_list(const T& entities, EntityType type);
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