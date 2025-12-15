#include "mesh.h"
#include "esphome/core/log.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

#ifdef IS_NODE
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

namespace esphome {
namespace esp_mesh {

static const char *const TAG = "mesh";
static EspMesh *global_mesh = nullptr;

// --- IMPLEMENTAZIONE SETTERS ---
void EspMesh::set_mesh_id(const std::string &id) {
  // Calcoliamo l'hash subito, quando Python ci passa l'ID
  this->net_id_hash_ = djb2_hash(id);
}

void EspMesh::set_pmk(const std::string &pmk) {
  this->pmk_ = pmk;
}

void EspMesh::set_channel(uint8_t channel) {
  this->current_scan_ch_ = channel;
}

uint32_t EspMesh::djb2_hash(const std::string &s) {
  uint32_t h = 5381;
  for (char c : s) {
    h = ((h << 5) + h) + c;
  }
  return h;
}

float EspMesh::get_setup_priority() const {
#ifdef IS_NODE
  return setup_priority::WIFI;
#else
  return setup_priority::AFTER_WIFI;
#endif
}

void EspMesh::setup() {
  global_mesh = this;

#ifdef IS_NODE
  this->setup_bare_metal();
  if (esp_now_init() != ESP_OK) {
    this->mark_failed();
    return;
  }
  // Usiamo la variabile membro pmk_ popolata dal setter
  esp_now_set_pmk(reinterpret_cast<uint8_t *>(const_cast<char *>(this->pmk_.c_str())));
#endif

#ifdef IS_ROOT
  esp_wifi_get_mac(WIFI_IF_STA, this->my_mac_);
  if (esp_now_init() != ESP_OK) {
    this->mark_failed();
    return;
  }
  esp_now_set_pmk(reinterpret_cast<uint8_t *>(const_cast<char *>(this->pmk_.c_str())));
  this->hop_count_ = 0;
#endif

  esp_now_register_recv_cb([](const esp_now_recv_info_t *i, const uint8_t *d, int l) {
    if (global_mesh) {
      global_mesh->on_packet(i->src_addr, d, l, i->rx_ctrl ? i->rx_ctrl->rssi : 0);
    }
  });
  ESP_LOGI(TAG, "Mesh initialized. ID Hash: %08X", this->net_id_hash_);
}

void EspMesh::dump_config() {
  ESP_LOGCONFIG(TAG, "ESP-Mesh Configuration:");
  ESP_LOGCONFIG(TAG, "  Net ID Hash: %08X", this->net_id_hash_);
  ESP_LOGCONFIG(TAG, "  Max Peers: %d", MAX_PEERS);
#ifdef IS_ROOT
  ESP_LOGCONFIG(TAG, "  Role: ROOT (Gateway)");
  ESP_LOGCONFIG(TAG, "  MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", 
                this->my_mac_[0], this->my_mac_[1], this->my_mac_[2], 
                this->my_mac_[3], this->my_mac_[4], this->my_mac_[5]);
#else
  ESP_LOGCONFIG(TAG, "  Role: NODE (Sensor)");
  ESP_LOGCONFIG(TAG, "  Bare Metal WiFi: Active");
#endif
}

void EspMesh::loop() {
  uint32_t now = millis();

  // 1. ANNOUNCE PROPAGATION
  if (this->hop_count_ != 0xFF) {
#ifdef IS_ROOT
    if (now - this->last_announce_ > 5000) {
      this->last_announce_ = now;
      // Broadcast Announce Hop 0
      uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      MeshHeader h;
      h.type = PKT_ANNOUNCE;
      h.net_id = this->net_id_hash_;
      h.ttl = 1;
      memcpy(h.src, this->my_mac_, 6);
      memcpy(h.dst, bcast, 6);
      uint8_t hop = 0;

      uint8_t buf[sizeof(MeshHeader) + 1];
      memcpy(buf, &h, sizeof(MeshHeader));
      buf[sizeof(MeshHeader)] = hop;
      this->send_raw(bcast, buf, sizeof(buf));
    }
#endif

#ifdef IS_NODE
    // Rebroadcast Announce (Repeater Logic)
    if (now - this->last_announce_sent_ > 5000) {
      this->last_announce_sent_ = now;
      uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      MeshHeader h;
      h.type = PKT_ANNOUNCE;
      h.net_id = this->net_id_hash_;
      h.ttl = 1;
      memcpy(h.src, this->my_mac_, 6);
      memcpy(h.dst, bcast, 6);
      uint8_t my_h = this->hop_count_;

      uint8_t buf[sizeof(MeshHeader) + 1];
      memcpy(buf, &h, sizeof(MeshHeader));
      buf[sizeof(MeshHeader)] = my_h;
      this->send_raw(bcast, buf, sizeof(buf));
    }
#endif
  }

  // 2. SCANNING LOGIC (NODE ONLY)
#ifdef IS_NODE
  if (this->hop_count_ == 0xFF) {
    if (now - this->last_scan_step_ > 200) {
      this->last_scan_step_ = now;
      this->current_scan_ch_ = (this->current_scan_ch_ % 13) + 1;
      esp_wifi_set_channel(this->current_scan_ch_, WIFI_SECOND_CHAN_NONE);
      this->send_probe();
    }
  }
#endif

  // 3. ROUTE GARBAGE COLLECTOR (Every 60s)
  static uint32_t last_route_gc = 0;
  if (now - last_route_gc > 60000) {
    last_route_gc = now;
    for (auto it = this->routes_.begin(); it != this->routes_.end();) {
      // 5 minutes timeout
      if (now - it->second.last_seen > 300000) {
        it = this->routes_.erase(it);
      } else {
        ++it;
      }
    }
  }
}

void EspMesh::on_packet(const uint8_t *mac, const uint8_t *data, int len, int8_t rssi) {
  if (len < sizeof(MeshHeader))
    return;
  auto *h = reinterpret_cast<const MeshHeader *>(data);
  if (h->net_id != this->net_id_hash_)
    return;

  // 1. REVERSE PATH LEARNING
  if (memcmp(h->src, mac, 6) != 0) {
    std::string src_s(reinterpret_cast<const char *>(h->src), 6);
    RouteInfo r;
    memcpy(r.next_hop, mac, 6);
    r.last_seen = millis();
    this->routes_[src_s] = r;
  }

  // 2. HANDLE ANNOUNCE
  if (h->type == PKT_ANNOUNCE) {
    uint8_t remote_hop = data[sizeof(MeshHeader)];
#ifdef IS_NODE
    if (this->hop_count_ == 0xFF || remote_hop + 1 < this->hop_count_) {
      this->hop_count_ = remote_hop + 1;
      memcpy(this->parent_mac_, h->src, 6);
      ESP_LOGI(TAG, "Parent Found: %02X.. (Hop %d) Ch:%d", mac[0], this->hop_count_,
               this->current_scan_ch_);
      this->scan_local_entities();
    }
#endif
    return;
  }

  // 3. ROUTING DECISION
  bool is_virtual_root = true;
  for (int i = 0; i < 6; i++) {
    if (h->dst[i] != 0)
      is_virtual_root = false;
  }

  bool is_for_me = (memcmp(h->dst, this->my_mac_, 6) == 0);

#ifdef IS_ROOT
  if (is_virtual_root)
    is_for_me = true;
#endif

  bool is_bcast = (h->dst[0] == 0xFF);

  if (is_for_me || is_bcast) {
// PROCESS PAYLOAD
#ifdef IS_ROOT
    if (h->type == PKT_REG) {
      this->handle_reg(h->src, reinterpret_cast<const RegPayload *>(data + sizeof(MeshHeader)));
    } else if (h->type == PKT_DATA) {
      this->handle_data(h->src, data + sizeof(MeshHeader));
    }
#endif
  }

  // FORWARDING
  if (!is_for_me && !is_bcast && h->ttl > 0) {
    uint8_t buf[250];
    if (len > 250) return; 
    memcpy(buf, data, len);
    
    auto *mutable_h = reinterpret_cast<MeshHeader *>(buf);
    mutable_h->ttl--;
    
    this->route_packet(mutable_h, buf + sizeof(MeshHeader), len - sizeof(MeshHeader));
  }
}

void EspMesh::route_packet(MeshHeader *h, const uint8_t *payload, int len) {
  uint8_t next_hop[6];

  if (h->dst[0] == 0xFF) {
    memset(next_hop, 0xFF, 6);
  } else {
    std::string dst_s(reinterpret_cast<const char *>(h->dst), 6);
    if (this->routes_.count(dst_s)) {
      memcpy(next_hop, this->routes_[dst_s].next_hop, 6);
    } else {
// Upstream
#ifdef IS_NODE
      if (this->hop_count_ != 0xFF) {
        memcpy(next_hop, this->parent_mac_, 6);
      } else {
        return;
      }
#else
      return;  // Root has no parent
#endif
    }
  }

  uint8_t buf[250];
  if (sizeof(MeshHeader) + len > 250)
    return;

  memcpy(buf, h, sizeof(MeshHeader));
  memcpy(buf + sizeof(MeshHeader), payload, len);

  this->send_raw(next_hop, buf, sizeof(MeshHeader) + len);
}

// --- PEER MANAGEMENT ---
void EspMesh::ensure_peer_slot(const uint8_t *mac) {
  if (esp_now_is_peer_exist(mac)) {
    std::string s(reinterpret_cast<const char *>(mac), 6);
    this->peer_lru_.remove(s);
    this->peer_lru_.push_back(s);
    return;
  }

  if (this->peer_lru_.size() >= MAX_PEERS) {
    std::string victim_s = this->peer_lru_.front();
#ifdef IS_NODE
    if (this->hop_count_ != 0xFF && memcmp(victim_s.c_str(), this->parent_mac_, 6) == 0) {
      if (this->peer_lru_.size() > 1) {
        auto it = this->peer_lru_.begin();
        it++;
        victim_s = *it;
      } else {
        return;
      }
    }
#endif

    esp_now_del_peer(reinterpret_cast<const uint8_t *>(victim_s.c_str()));
    this->peer_lru_.remove(victim_s);
    ESP_LOGD(TAG, "Evicted peer to make space");
  }

  esp_now_peer_info_t pi = {};
  memcpy(pi.peer_addr, mac, 6);
  pi.channel = (this->hop_count_ == 0xFF) ? this->current_scan_ch_ : 0;
  pi.encrypt = true;
  this->derive_lmk(mac, pi.lmk);

  if (esp_now_add_peer(&pi) == ESP_OK) {
    this->peer_lru_.push_back(std::string(reinterpret_cast<const char *>(mac), 6));
  }
}

void EspMesh::send_raw(const uint8_t *next_hop, const uint8_t *data, int len) {
  bool is_bcast = (next_hop[0] == 0xFF);

  if (!is_bcast) {
    this->ensure_peer_slot(next_hop);
  } else {
    if (!esp_now_is_peer_exist(next_hop)) {
      esp_now_peer_info_t pi = {};
      memcpy(pi.peer_addr, next_hop, 6);
      pi.encrypt = false;
      esp_now_add_peer(&pi);
    }
  }

  esp_now_send(next_hop, data, len);
}

void EspMesh::derive_lmk(const uint8_t *mac, uint8_t *lmk) {
  for (int i = 0; i < 16; i++) {
    lmk[i] = this->pmk_[i] ^ mac[i % 6];
  }
}

#ifdef IS_NODE
void EspMesh::setup_bare_metal() {
  nvs_flash_init();
  esp_netif_init();
  esp_event_loop_create_default();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_channel(this->current_scan_ch_, WIFI_SECOND_CHAN_NONE);
  esp_wifi_start();
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_get_mac(WIFI_IF_STA, this->my_mac_);
}

void EspMesh::send_probe() {
  uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  MeshHeader h;
  h.type = PKT_PROBE;
  h.net_id = this->net_id_hash_;
  h.ttl = 1;
  memcpy(h.src, this->my_mac_, 6);
  memcpy(h.dst, bcast, 6);
  this->send_raw(bcast, reinterpret_cast<uint8_t *>(&h), sizeof(h));
}

void EspMesh::scan_local_entities() {
  uint8_t root_dst[6] = {0};

  for (auto *s : App.get_sensors()) {
    RegPayload p;
    p.entity_hash = s->get_object_id_hash();
    p.type_id = 'S';
    strncpy(p.name, s->get_name().c_str(), 24);
    strncpy(p.unit, s->get_unit_of_measurement().c_str(), 8);
    strncpy(p.dev_class, s->get_device_class().c_str(), 16);

    MeshHeader h;
    h.type = PKT_REG;
    h.net_id = this->net_id_hash_;
    h.ttl = 10;
    memcpy(h.src, this->my_mac_, 6);  // <-- CORRETTO
    memcpy(h.dst, root_dst, 6);       // <-- CORRETTO

    this->route_packet(&h, reinterpret_cast<uint8_t *>(&p), sizeof(p));
    delay(50); 

    s->add_on_state_callback([this, s](float val) {
      MeshHeader dh;
      dh.type = PKT_DATA;
      dh.net_id = this->net_id_hash_;
      dh.ttl = 10;
      memcpy(dh.src, this->my_mac_, 6); // <-- CORRETTO
      memset(dh.dst, 0, 6);

      uint8_t pl[8];
      uint32_t hash = s->get_object_id_hash();
      memcpy(pl, &hash, 4);
      memcpy(pl + 4, &val, 4);

      this->route_packet(&dh, pl, 8);
    });
  }
}
#endif

#ifdef IS_ROOT
void EspMesh::handle_reg(const uint8_t *origin, const RegPayload *p) {
  if (!this->mqtt_)
    return;
  char m[13];
  sprintf(m, "%02X%02X%02X%02X%02X%02X", origin[0], origin[1], origin[2], origin[3], origin[4],
          origin[5]);
  std::string uid = std::string(m) + "_" + to_string(p->entity_hash);

  std::string top = "homeassistant/sensor/" + uid + "/config";
  std::string stat = "mesh_gw/" + uid + "/state";
  std::string j = "{\"name\":\"" + std::string(p->name) + "\",\"uniq_id\":\"" + uid +
                  "\",\"stat_t\":\"" + stat + "\",\"dev\":{\"ids\":[\"" + std::string(m) +
                  "\"],\"name\":\"Node " + std::string(m) + "\"}}";
  this->mqtt_->publish(top, j, 0, true);
}
void EspMesh::handle_data(const uint8_t *origin, const uint8_t *payload) {
  if (!this->mqtt_)
    return;
  uint32_t hash;
  float val;
  memcpy(&hash, payload, 4);
  memcpy(&val, payload + 4, 4);
  char m[13];
  sprintf(m, "%02X%02X%02X%02X%02X%02X", origin[0], origin[1], origin[2], origin[3], origin[4],
          origin[5]);
  std::string uid = std::string(m) + "_" + to_string(hash);
  char vs[16];
  sprintf(vs, "%.2f", val);
  this->mqtt_->publish("mesh_gw/" + uid + "/state", vs);
}
#endif

}  // namespace esp_mesh
}  // namespace esphome