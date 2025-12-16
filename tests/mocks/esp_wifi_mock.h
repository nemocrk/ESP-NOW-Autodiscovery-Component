#pragma once

/**
 * Mock Header for ESP-WiFi API
 * 
 * Fornisce stub delle funzioni ESP WiFi per permettere il testing
 * su Linux senza dipendenze hardware ESP32.
 * 
 * Implementa:
 *   - esp_wifi_get_mac()
 *   - esp_wifi_set_channel()
 *   - esp_wifi_init()
 *   - esp_wifi_set_mode()
 *   - esp_wifi_start()
 *   - esp_wifi_set_ps()
 */

#include <cstdint>
#include <cstring>
#include <map>

// ============================================
// ESP-WiFi Type Definitions (Stubs)
// ============================================

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1

typedef enum {
    WIFI_IF_STA = 0,
    WIFI_IF_AP = 1,
} wifi_interface_t;

typedef enum {
    WIFI_MODE_NULL = 0,
    WIFI_MODE_STA = 1,
    WIFI_MODE_AP = 2,
    WIFI_MODE_APSTA = 3,
} wifi_mode_t;

typedef enum {
    WIFI_SECOND_CHAN_NONE = 0,
    WIFI_SECOND_CHAN_ABOVE = 1,
    WIFI_SECOND_CHAN_BELOW = 2,
} wifi_second_chan_t;

typedef enum {
    WIFI_PS_NONE = 0,
    WIFI_PS_MIN_MODEM = 1,
    WIFI_PS_MAX_MODEM = 2,
} wifi_ps_type_t;

struct wifi_init_config_t {
    uint8_t dummy;
};

#define WIFI_INIT_CONFIG_DEFAULT() \
    { .dummy = 0 }

// ============================================
// Global Mock State
// ============================================

class EspWiFiMock {
public:
    static EspWiFiMock& instance() {
        static EspWiFiMock inst;
        return inst;
    }

    // Mock state
    uint8_t mac_sta[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t mac_ap[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint8_t current_channel = 1;
    wifi_mode_t current_mode = WIFI_MODE_NULL;
    bool initialized = false;
    bool started = false;
    wifi_ps_type_t ps_mode = WIFI_PS_NONE;

    void reset() {
        memset(mac_sta, 0, 6);
        memset(mac_ap, 0, 6);
        current_channel = 1;
        current_mode = WIFI_MODE_NULL;
        initialized = false;
        started = false;
        ps_mode = WIFI_PS_NONE;
    }
};

// ============================================
// Mock Function Implementations
// ============================================

inline esp_err_t esp_wifi_get_mac(wifi_interface_t ifx, uint8_t *mac) {
    if (!mac) return ESP_FAIL;

    auto& mock = EspWiFiMock::instance();
    if (ifx == WIFI_IF_STA) {
        memcpy(mac, mock.mac_sta, 6);
    } else if (ifx == WIFI_IF_AP) {
        memcpy(mac, mock.mac_ap, 6);
    } else {
        return ESP_FAIL;
    }
    return ESP_OK;
}

inline esp_err_t esp_wifi_set_channel(uint8_t channel,
                                       wifi_second_chan_t second) {
    if (channel < 1 || channel > 13) return ESP_FAIL;
    EspWiFiMock::instance().current_channel = channel;
    return ESP_OK;
}

inline esp_err_t esp_wifi_init(const wifi_init_config_t *config) {
    EspWiFiMock::instance().initialized = true;
    return ESP_OK;
}

inline esp_err_t esp_wifi_set_mode(wifi_mode_t mode) {
    EspWiFiMock::instance().current_mode = mode;
    return ESP_OK;
}

inline esp_err_t esp_wifi_start() {
    EspWiFiMock::instance().started = true;
    return ESP_OK;
}

inline esp_err_t esp_wifi_set_ps(wifi_ps_type_t type) {
    EspWiFiMock::instance().ps_mode = type;
    return ESP_OK;
}
