#pragma once

#include <gmock/gmock.h>
#include <cstdint>

/**
 * Mock per le funzioni WiFi di basso livello.
 */

class MockWiFi {
public:
    MOCK_METHOD(esp_err_t, esp_wifi_init, (), (const));
    MOCK_METHOD(esp_err_t, esp_wifi_deinit, (), (const));
    MOCK_METHOD(esp_err_t, esp_wifi_get_mac, (uint8_t role, uint8_t* mac), (const));
    MOCK_METHOD(esp_err_t, esp_wifi_set_channel, (uint8_t primary, uint8_t type), (const));
};

extern MockWiFi* g_mock_wifi;
