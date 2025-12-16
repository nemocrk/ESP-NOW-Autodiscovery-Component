#pragma once

#include <gmock/gmock.h>
#include <cstdint>
#include <cstring>

/**
 * Mock per le funzioni ESP-NOW basse livello.
 * Simula il comportamento del modulo ESP-NOW hardware.
 */

class MockEspNow {
public:
    MOCK_METHOD(esp_err_t, esp_now_init, (), (const));
    MOCK_METHOD(esp_err_t, esp_now_deinit, (), (const));
    MOCK_METHOD(esp_err_t, esp_now_send, (const uint8_t* mac_addr, const uint8_t* data, size_t len), (const));
    MOCK_METHOD(esp_err_t, esp_now_add_peer, (const uint8_t* mac_addr, bool encrypt, const uint8_t* lmk), (const));
    MOCK_METHOD(esp_err_t, esp_now_del_peer, (const uint8_t* mac_addr), (const));
    MOCK_METHOD(bool, esp_now_is_peer_exist, (const uint8_t* mac_addr), (const));
    MOCK_METHOD(esp_err_t, esp_now_register_recv_cb, (void (*cb)(const uint8_t*, const uint8_t*, int, uint8_t, size_t)), (const));
};

extern MockEspNow* g_mock_esp_now;

// Funzioni wrapper per linkare i test
inline esp_err_t mock_esp_now_init() {
    return g_mock_esp_now->esp_now_init();
}

inline esp_err_t mock_esp_now_send(const uint8_t* mac, const uint8_t* data, size_t len) {
    return g_mock_esp_now->esp_now_send(mac, data, len);
}
