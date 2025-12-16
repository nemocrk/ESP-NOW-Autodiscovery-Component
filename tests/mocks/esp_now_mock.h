#pragma once

/**
 * Mock Header for ESP-NOW API
 * 
 * Fornisce stub delle funzioni ESP-NOW per permettere il testing
 * su Linux senza dipendenze hardware ESP32.
 * 
 * Implementa:
 *   - esp_now_init()
 *   - esp_now_set_pmk()
 *   - esp_now_register_recv_cb()
 *   - esp_now_is_peer_exist()
 *   - esp_now_add_peer()
 *   - esp_now_del_peer()
 *   - esp_now_send()
 */

#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <vector>

// ============================================
// ESP-NOW Type Definitions (Stubs)
// ============================================

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1

struct esp_now_peer_info_t {
    uint8_t peer_addr[6];
    uint8_t channel;
    uint8_t encrypt;
    uint8_t lmk[16];
};

struct esp_now_recv_info_t {
    uint8_t *src_addr;
    uint8_t *dst_addr;
    struct {
        int8_t rssi;
    } *rx_ctrl;
};

typedef void (*esp_now_recv_cb_t)(const esp_now_recv_info_t *info,
                                   const uint8_t *data, int len);

// ============================================
// Global Mock State
// ============================================

class EspNowMock {
public:
    static EspNowMock& instance() {
        static EspNowMock inst;
        return inst;
    }

    // Mock state
    std::map<std::string, esp_now_peer_info_t> peers;
    esp_now_recv_cb_t recv_callback = nullptr;
    uint8_t pmk[16] = {0};
    bool initialized = false;

    // Helper: MAC to string
    static std::string mac_to_string(const uint8_t *mac) {
        char buf[18];
        snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return std::string(buf);
    }

    void reset() {
        peers.clear();
        recv_callback = nullptr;
        memset(pmk, 0, 16);
        initialized = false;
    }
};

// ============================================
// Mock Function Implementations
// ============================================

inline esp_err_t esp_now_init() {
    EspNowMock::instance().initialized = true;
    return ESP_OK;
}

inline esp_err_t esp_now_set_pmk(const uint8_t *pmk) {
    if (!pmk) return ESP_FAIL;
    memcpy(EspNowMock::instance().pmk, pmk, 16);
    return ESP_OK;
}

inline esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t cb) {
    EspNowMock::instance().recv_callback = cb;
    return ESP_OK;
}

inline bool esp_now_is_peer_exist(const uint8_t *peer_addr) {
    auto& mock = EspNowMock::instance();
    std::string key = EspNowMock::mac_to_string(peer_addr);
    return mock.peers.find(key) != mock.peers.end();
}

inline esp_err_t esp_now_add_peer(const esp_now_peer_info_t *peer_info) {
    if (!peer_info) return ESP_FAIL;
    auto& mock = EspNowMock::instance();
    std::string key = EspNowMock::mac_to_string(peer_info->peer_addr);
    mock.peers[key] = *peer_info;
    return ESP_OK;
}

inline esp_err_t esp_now_del_peer(const uint8_t *peer_addr) {
    auto& mock = EspNowMock::instance();
    std::string key = EspNowMock::mac_to_string(peer_addr);
    auto it = mock.peers.find(key);
    if (it != mock.peers.end()) {
        mock.peers.erase(it);
        return ESP_OK;
    }
    return ESP_FAIL;
}

inline esp_err_t esp_now_send(const uint8_t *peer_addr, const uint8_t *data,
                               size_t len) {
    // Mock: just record that send was called
    // In real tests, we can intercept this to verify data
    if (!data || len == 0) return ESP_FAIL;
    return ESP_OK;
}

// Test helper: simulate receiving a packet
inline void test_simulate_recv(const uint8_t *src_addr, const uint8_t *dst_addr,
                               const uint8_t *data, int len, int8_t rssi) {
    auto& mock = EspNowMock::instance();
    if (!mock.recv_callback) return;

    // Allocate temporary structures for callback
    esp_now_recv_info_t info;
    uint8_t src_copy[6], dst_copy[6];
    struct {
        int8_t rssi;
    } rx_ctrl_data;

    memcpy(src_copy, src_addr, 6);
    memcpy(dst_copy, dst_addr, 6);
    info.src_addr = src_copy;
    info.dst_addr = dst_copy;
    info.rx_ctrl = &rx_ctrl_data;
    rx_ctrl_data.rssi = rssi;

    mock.recv_callback(&info, data, len);
}
