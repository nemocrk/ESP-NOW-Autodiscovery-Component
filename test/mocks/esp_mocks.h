#pragma once

/**
 * Complete ESP-IDF and ESPHome Mocks for Unit Testing
 * 
 * Questo file permette di compilare mesh.cpp su Linux
 * sostituendo tutte le API hardware con mock.
 */

#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <functional>
#include <map>

// ============================================
// ESP-IDF Type Definitions
// ============================================

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1

// ============================================
// WiFi Mocks
// ============================================

typedef enum {
    WIFI_MODE_NULL = 0,
    WIFI_MODE_STA,
    WIFI_MODE_AP,
    WIFI_MODE_APSTA,
} wifi_mode_t;

typedef enum {
    WIFI_IF_STA = 0,
    WIFI_IF_AP,
} wifi_interface_t;

typedef enum {
    WIFI_SECOND_CHAN_NONE = 0,
} wifi_second_chan_t;

typedef enum {
    WIFI_PS_NONE = 0,
} wifi_ps_type_t;

struct wifi_init_config_t {
    int dummy;
};

#define WIFI_INIT_CONFIG_DEFAULT() {0}

// Global state
namespace mock_state {
    inline uint8_t wifi_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    inline uint8_t wifi_channel = 1;
    inline wifi_mode_t wifi_mode = WIFI_MODE_NULL;
}

inline esp_err_t esp_wifi_init(const wifi_init_config_t *config) {
    (void)config;
    return ESP_OK;
}

inline esp_err_t esp_wifi_set_mode(wifi_mode_t mode) {
    mock_state::wifi_mode = mode;
    return ESP_OK;
}

inline esp_err_t esp_wifi_start() {
    return ESP_OK;
}

inline esp_err_t esp_wifi_set_ps(wifi_ps_type_t type) {
    (void)type;
    return ESP_OK;
}

inline esp_err_t esp_wifi_set_channel(uint8_t primary, wifi_second_chan_t second) {
    (void)second;
    mock_state::wifi_channel = primary;
    return ESP_OK;
}

inline esp_err_t esp_wifi_get_mac(wifi_interface_t ifx, uint8_t *mac) {
    (void)ifx;
    memcpy(mac, mock_state::wifi_mac, 6);
    return ESP_OK;
}

// ============================================
// ESP-NOW Mocks
// ============================================

struct esp_now_peer_info_t {
    uint8_t peer_addr[6];
    uint8_t lmk[16];
    uint8_t channel;
    bool encrypt;
};

struct esp_now_recv_info_t {
    uint8_t *src_addr;
    uint8_t *dst_addr;
    struct {
        int8_t rssi;
    } *rx_ctrl;
};

typedef void (*esp_now_recv_cb_t)(const esp_now_recv_info_t *info, const uint8_t *data, int len);
typedef void (*esp_now_send_cb_t)(const uint8_t *mac_addr, esp_err_t status);

// Global mock state for ESP-NOW
namespace esp_now_mock {
    inline bool initialized = false;
    inline uint8_t pmk[16] = {0};
    inline esp_now_recv_cb_t recv_callback = nullptr;
    inline esp_now_send_cb_t send_callback = nullptr;
    inline std::map<std::string, esp_now_peer_info_t> peers;
    
    // Test helpers
    inline std::vector<std::vector<uint8_t>> sent_packets;
    inline std::vector<std::string> sent_to_macs;
}

inline std::string mac_to_str(const uint8_t *mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
}

inline esp_err_t esp_now_init() {
    esp_now_mock::initialized = true;
    return ESP_OK;
}

inline esp_err_t esp_now_deinit() {
    esp_now_mock::initialized = false;
    return ESP_OK;
}

inline esp_err_t esp_now_set_pmk(const uint8_t *pmk) {
    if (!pmk) return ESP_FAIL;
    memcpy(esp_now_mock::pmk, pmk, 16);
    return ESP_OK;
}

inline esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t cb) {
    esp_now_mock::recv_callback = cb;
    return ESP_OK;
}

inline esp_err_t esp_now_register_send_cb(esp_now_send_cb_t cb) {
    esp_now_mock::send_callback = cb;
    return ESP_OK;
}

inline bool esp_now_is_peer_exist(const uint8_t *peer_addr) {
    std::string key = mac_to_str(peer_addr);
    return esp_now_mock::peers.find(key) != esp_now_mock::peers.end();
}

inline esp_err_t esp_now_add_peer(const esp_now_peer_info_t *peer) {
    if (!peer) return ESP_FAIL;
    std::string key = mac_to_str(peer->peer_addr);
    esp_now_mock::peers[key] = *peer;
    return ESP_OK;
}

inline esp_err_t esp_now_del_peer(const uint8_t *peer_addr) {
    std::string key = mac_to_str(peer_addr);
    auto it = esp_now_mock::peers.find(key);
    if (it != esp_now_mock::peers.end()) {
        esp_now_mock::peers.erase(it);
        return ESP_OK;
    }
    return ESP_FAIL;
}

inline esp_err_t esp_now_send(const uint8_t *peer_addr, const uint8_t *data, size_t len) {
    if (!peer_addr || !data) return ESP_FAIL;
    
    // Log for test verification
    esp_now_mock::sent_packets.push_back(std::vector<uint8_t>(data, data + len));
    esp_now_mock::sent_to_macs.push_back(mac_to_str(peer_addr));
    
    // Simulate send callback
    if (esp_now_mock::send_callback) {
        esp_now_mock::send_callback(peer_addr, ESP_OK);
    }
    
    return ESP_OK;
}

// Test helper: simulate receiving packet
inline void test_simulate_recv(const uint8_t *src, const uint8_t *dst, 
                               const uint8_t *data, int len, int8_t rssi = -60) {
    if (!esp_now_mock::recv_callback) return;
    
    uint8_t src_copy[6], dst_copy[6];
    memcpy(src_copy, src, 6);
    memcpy(dst_copy, dst, 6);
    
    struct {
        int8_t rssi;
    } rx_ctrl;
    rx_ctrl.rssi = rssi;
    
    esp_now_recv_info_t info;
    info.src_addr = src_copy;
    info.dst_addr = dst_copy;
    info.rx_ctrl = &rx_ctrl;
    
    esp_now_mock::recv_callback(&info, data, len);
}

// ============================================
// NVS (Non-Volatile Storage) Mocks
// ============================================

inline esp_err_t nvs_flash_init() {
    return ESP_OK;
}

// ============================================
// Network Interface Mocks
// ============================================

inline esp_err_t esp_netif_init() {
    return ESP_OK;
}

inline esp_err_t esp_event_loop_create_default() {
    return ESP_OK;
}

// ============================================
// Logging Mocks
// ============================================

#define ESP_LOGI(tag, format, ...) \
    printf("[INFO][%s] " format "\n", tag, ##__VA_ARGS__)

#define ESP_LOGD(tag, format, ...) \
    printf("[DEBUG][%s] " format "\n", tag, ##__VA_ARGS__)

#define ESP_LOGW(tag, format, ...) \
    printf("[WARN][%s] " format "\n", tag, ##__VA_ARGS__)

#define ESP_LOGE(tag, format, ...) \
    fprintf(stderr, "[ERROR][%s] " format "\n", tag, ##__VA_ARGS__)

#define ESP_LOGCONFIG(tag, format, ...) \
    printf("[CONFIG][%s] " format "\n", tag, ##__VA_ARGS__)

// ============================================
// ESPHome Core Mocks
// ============================================

namespace esphome {

class Component {
public:
    virtual ~Component() = default;
    virtual void setup() {}
    virtual void loop() {}
    virtual void dump_config() {}
    virtual float get_setup_priority() const { return 0.0f; }
    
    void mark_failed() { failed_ = true; }
    bool is_failed() const { return failed_; }
    
protected:
    bool failed_ = false;
};

namespace setup_priority {
    constexpr float HARDWARE = 50.0f;
    constexpr float WIFI = 4.0f;
    constexpr float AFTER_WIFI = -50.0f;
}

// Time functions
inline uint32_t millis() {
    static uint32_t fake_millis = 0;
    return fake_millis++;
}

inline void delay(uint32_t ms) {
    (void)ms; // No real delay in tests
}

} // namespace esphome

// ============================================
// Test Helper Functions
// ============================================

inline void mock_reset_all() {
    esp_now_mock::initialized = false;
    esp_now_mock::recv_callback = nullptr;
    esp_now_mock::send_callback = nullptr;
    esp_now_mock::peers.clear();
    esp_now_mock::sent_packets.clear();
    esp_now_mock::sent_to_macs.clear();
    memset(esp_now_mock::pmk, 0, 16);
    
    mock_state::wifi_channel = 1;
    mock_state::wifi_mode = WIFI_MODE_NULL;
}
