#pragma once

/**
 * Mock Header for ESPHome Core API
 * 
 * Fornisce stub per ESPHome logging e utility functions
 * per permettere il testing su Linux.
 * 
 * Implementa:
 *   - ESP_LOGI() / ESP_LOGCONFIG() / ESP_LOGD() / ESP_LOGW()
 *   - Component base class
 *   - setup_priority enum
 *   - millis() / delay()
 */

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <chrono>

// ============================================
// Logging Macros (No-op for testing)
// ============================================

#define ESP_LOGI(tag, format, ...) \
    do { \
        fprintf(stdout, "[INFO][%s] " format "\n", tag, ##__VA_ARGS__); \
        fflush(stdout); \
    } while (0)

#define ESP_LOGCONFIG(tag, format, ...) \
    do { \
        fprintf(stdout, "[CONFIG][%s] " format "\n", tag, ##__VA_ARGS__); \
        fflush(stdout); \
    } while (0)

#define ESP_LOGD(tag, format, ...) \
    do { \
        fprintf(stdout, "[DEBUG][%s] " format "\n", tag, ##__VA_ARGS__); \
        fflush(stdout); \
    } while (0)

#define ESP_LOGW(tag, format, ...) \
    do { \
        fprintf(stderr, "[WARN][%s] " format "\n", tag, ##__VA_ARGS__); \
        fflush(stderr); \
    } while (0)

#define ESP_LOGE(tag, format, ...) \
    do { \
        fprintf(stderr, "[ERROR][%s] " format "\n", tag, ##__VA_ARGS__); \
        fflush(stderr); \
    } while (0)

// ============================================
// Time Functions
// ============================================

class MockTimer {
public:
    static MockTimer& instance() {
        static MockTimer inst;
        return inst;
    }

    uint32_t start_time_ms = 0;
    uint32_t current_time_ms = 0;

    void reset() {
        auto now = std::chrono::steady_clock::now();
        start_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch())
                            .count();
        current_time_ms = 0;
    }

    uint32_t get_time_ms() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   now.time_since_epoch())
            .count() -
            start_time_ms + current_time_ms;
    }
};

inline uint32_t millis() {
    return MockTimer::instance().get_time_ms();
}

inline void delay(uint32_t ms) {
    // In tests, we don't actually sleep - just advance mock time
    MockTimer::instance().current_time_ms += ms;
}

// ============================================
// Setup Priority Enum
// ============================================

namespace setup_priority {
constexpr float BEFORE_HARDWARE = 100.0f;
constexpr float HARDWARE = 50.0f;
constexpr float BUS = 40.0f;
constexpr float IO = 30.0f;
constexpr float WIFI = 4.0f;
constexpr float AFTER_WIFI = -50.0f;
constexpr float AFTER_CONNECTION = -100.0f;
}  // namespace setup_priority

// ============================================
// Component Base Class (Stub)
// ============================================

class Component {
public:
    virtual ~Component() = default;

    virtual void setup() {}
    virtual void loop() {}
    virtual void dump_config() {}
    virtual float get_setup_priority() const {
        return setup_priority::IO;
    }

    void mark_failed() {
        failed_ = true;
    }

    bool is_failed() const {
        return failed_;
    }

    void set_setup_priority(float priority) {
        setup_priority_ = priority;
    }

protected:
    bool failed_ = false;
    float setup_priority_ = setup_priority::IO;
};

// ============================================
// Utility Macros
// ============================================

#define to_string std::to_string

// ============================================
// Mock Time Control (for tests)
// ============================================

inline void test_reset_timer() {
    MockTimer::instance().reset();
}

inline void test_advance_time_ms(uint32_t ms) {
    MockTimer::instance().current_time_ms += ms;
}

inline uint32_t test_get_current_time_ms() {
    return MockTimer::instance().get_time_ms();
}
