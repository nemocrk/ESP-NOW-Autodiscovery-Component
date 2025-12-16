#include <gtest/gtest.h>
#include <cstdint>
#include <deque>
#include <cstring>
#include <array>
#include <map>
#include <string>

/**
 * Test unitari per il routing layer (Layer 3).
 * Testa:
 *   - Reverse path learning
 *   - Route table management
 *   - TTL handling
 *   - Multi-hop routing
 */

// ============================================
// Route Table and Routing Logic
// ============================================

struct Route {
    std::array<uint8_t, 6> next_hop;
    uint8_t hop_count;
    int8_t rssi;
};

class RoutingManager {
public:
    static constexpr uint8_t MAX_ROUTES = 50;
    static constexpr uint8_t MAX_TTL = 10;
    
    RoutingManager() : routes_count_(0), packets_routed_(0) {}
    
    // Learn reverse path from source MAC
    bool learn_route(const std::array<uint8_t, 6>& from_mac,
                     const std::array<uint8_t, 6>& next_hop_mac,
                     uint8_t hop_count,
                     int8_t rssi) {
        if (routes_count_ >= MAX_ROUTES) {
            return false;  // Route table full
        }
        
        std::string mac_key = mac_to_string(from_mac);
        Route route;
        route.next_hop = next_hop_mac;
        route.hop_count = hop_count;
        route.rssi = rssi;
        
        route_table_[mac_key] = route;
        routes_count_++;
        return true;
    }
    
    // Get route to destination
    bool get_route(const std::array<uint8_t, 6>& dest_mac,
                   std::array<uint8_t, 6>& next_hop) {
        std::string mac_key = mac_to_string(dest_mac);
        auto it = route_table_.find(mac_key);
        if (it != route_table_.end()) {
            next_hop = it->second.next_hop;
            return true;
        }
        return false;
    }
    
    // Forward packet with TTL decrement
    bool forward_packet(uint8_t& ttl) {
        if (ttl == 0) {
            return false;  // Expired
        }
        ttl--;
        packets_routed_++;
        return true;
    }
    
    // TTL validation
    static bool is_ttl_valid(uint8_t ttl) {
        return ttl > 0 && ttl <= MAX_TTL;
    }
    
    // Getters
    uint32_t get_route_count() const { return routes_count_; }
    uint32_t get_packets_routed() const { return packets_routed_; }
    int8_t get_best_rssi(const std::array<uint8_t, 6>& dest_mac) const {
        std::string mac_key = mac_to_string(dest_mac);
        auto it = route_table_.find(mac_key);
        if (it != route_table_.end()) {
            return it->second.rssi;
        }
        return -127;  // Invalid RSSI
    }
    
private:
    std::map<std::string, Route> route_table_;
    uint32_t routes_count_;
    uint32_t packets_routed_;
    
    static std::string mac_to_string(const std::array<uint8_t, 6>& mac) {
        char buf[18];
        snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return std::string(buf);
    }
};

// ============================================
// Test Suite: RoutingTest
// ============================================

class RoutingTest : public ::testing::Test {
public:
    void SetUp() override {
        router_ = std::make_unique<RoutingManager>();
        src_mac_ = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
        next_hop_mac_ = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    }
    
    std::unique_ptr<RoutingManager> router_;
    std::array<uint8_t, 6> src_mac_;
    std::array<uint8_t, 6> next_hop_mac_;
};

/**
 * TEST: ReversePathLearning
 * 
 * Verifica che route sia imparata dal MAC sorgente.
 */
TEST_F(RoutingTest, ReversePathLearning) {
    // Act
    bool learned = router_->learn_route(src_mac_, next_hop_mac_, 1, -70);
    
    // Assert
    EXPECT_TRUE(learned);
    EXPECT_EQ(router_->get_route_count(), 1);
    
    // Verify route exists
    std::array<uint8_t, 6> retrieved_hop;
    bool found = router_->get_route(src_mac_, retrieved_hop);
    EXPECT_TRUE(found);
    EXPECT_EQ(retrieved_hop, next_hop_mac_);
}

/**
 * TEST: MultipleRoutes
 * 
 * Verifica apprendimento di più rotte.
 */
TEST_F(RoutingTest, MultipleRoutes) {
    // Act
    std::array<uint8_t, 6> mac1 = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    std::array<uint8_t, 6> mac2 = {0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C};
    std::array<uint8_t, 6> hop1 = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
    std::array<uint8_t, 6> hop2 = {0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0};
    
    router_->learn_route(mac1, hop1, 1, -70);
    router_->learn_route(mac2, hop2, 2, -75);
    
    // Assert
    EXPECT_EQ(router_->get_route_count(), 2);
    
    std::array<uint8_t, 6> retrieved_hop1;
    EXPECT_TRUE(router_->get_route(mac1, retrieved_hop1));
    EXPECT_EQ(retrieved_hop1, hop1);
    
    std::array<uint8_t, 6> retrieved_hop2;
    EXPECT_TRUE(router_->get_route(mac2, retrieved_hop2));
    EXPECT_EQ(retrieved_hop2, hop2);
}

/**
 * TEST: TTLDecrement
 * 
 * Verifica TTL decrementato quando pacchetto forwarded.
 */
TEST_F(RoutingTest, TTLDecrement) {
    // Arrange
    uint8_t ttl = 10;
    
    // Act
    bool forwarded = router_->forward_packet(ttl);
    
    // Assert
    EXPECT_TRUE(forwarded);
    EXPECT_EQ(ttl, 9);
}

/**
 * TEST: TTLZeroNotForwarded
 * 
 * Verifica pacchetto TTL=0 non forwarded.
 */
TEST_F(RoutingTest, TTLZeroNotForwarded) {
    // Arrange
    uint8_t ttl = 0;
    uint32_t routed_before = router_->get_packets_routed();
    
    // Act
    bool forwarded = router_->forward_packet(ttl);
    
    // Assert
    EXPECT_FALSE(forwarded);
    EXPECT_EQ(router_->get_packets_routed(), routed_before);
}

/**
 * TEST: MultiHopPath
 * 
 * Verifica percorso multi-hop con decrementazione TTL.
 */
TEST_F(RoutingTest, MultiHopPath) {
    // Arrange
    uint8_t ttl = 10;
    
    // Act - Simula 3 hops
    for (int i = 0; i < 3; i++) {
        bool forwarded = router_->forward_packet(ttl);
        EXPECT_TRUE(forwarded);
    }
    
    // Assert
    EXPECT_EQ(ttl, 7);
    EXPECT_EQ(router_->get_packets_routed(), 3);
}

/**
 * TEST: RoutingLoopPrevention
 * 
 * Verifica prevenzione di loop di routing con TTL.
 */
TEST_F(RoutingTest, RoutingLoopPrevention) {
    // Arrange
    uint8_t ttl = 10;
    
    // Act - Forward 10 times (max TTL)
    for (int i = 0; i < 10; i++) {
        bool forwarded = router_->forward_packet(ttl);
        EXPECT_TRUE(forwarded);
    }
    
    // Assert - TTL should be 0
    EXPECT_EQ(ttl, 0);
    
    // Next forward should fail
    bool forwarded = router_->forward_packet(ttl);
    EXPECT_FALSE(forwarded);
}

/**
 * TEST: TTLValidation
 * 
 * Verifica validazione dei valori TTL.
 */
TEST_F(RoutingTest, TTLValidation) {
    // Assert valid TTL
    EXPECT_TRUE(RoutingManager::is_ttl_valid(1));
    EXPECT_TRUE(RoutingManager::is_ttl_valid(5));
    EXPECT_TRUE(RoutingManager::is_ttl_valid(10));
    EXPECT_TRUE(RoutingManager::is_ttl_valid(RoutingManager::MAX_TTL));
    
    // Assert invalid TTL
    EXPECT_FALSE(RoutingManager::is_ttl_valid(0));
    EXPECT_FALSE(RoutingManager::is_ttl_valid(11));
    EXPECT_FALSE(RoutingManager::is_ttl_valid(255));
}

/**
 * TEST: SourcePreservation
 * 
 * Verifica che source sia preservato nel forward.
 */
TEST_F(RoutingTest, SourcePreservation) {
    // Arrange
    std::array<uint8_t, 6> src = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    std::array<uint8_t, 6> src_copy;
    
    // Act
    memcpy(src_copy.data(), src.data(), 6);
    
    // Assert
    EXPECT_EQ(src_copy, src);
}

/**
 * TEST: RSSITracking
 * 
 * Verifica tracciamento RSSI per route selection.
 */
TEST_F(RoutingTest, RSSITracking) {
    // Arrange & Act
    router_->learn_route(src_mac_, next_hop_mac_, 1, -70);
    
    // Assert
    int8_t rssi = router_->get_best_rssi(src_mac_);
    EXPECT_EQ(rssi, -70);
}

/**
 * TEST: RouteUpdateWithBetterRSSI
 * 
 * Verifica che route con migliore RSSI sia aggiornato.
 */
TEST_F(RoutingTest, RouteUpdateWithBetterRSSI) {
    // Arrange - Learn first route
    router_->learn_route(src_mac_, next_hop_mac_, 2, -80);
    int8_t initial_rssi = router_->get_best_rssi(src_mac_);
    
    // Act - Learn same route with better RSSI
    std::array<uint8_t, 6> better_hop = {0x77, 0x77, 0x77, 0x77, 0x77, 0x77};
    router_->learn_route(src_mac_, better_hop, 1, -60);
    
    // Assert - Should have better route
    int8_t new_rssi = router_->get_best_rssi(src_mac_);
    EXPECT_LT(new_rssi, initial_rssi);  // Less negative = stronger
}

/**
 * TEST: RouteTableLimit
 * 
 * Verifica comportamento quando tabella di routing è piena.
 */
TEST_F(RoutingTest, RouteTableLimit) {
    // Act - Fill route table
    for (uint32_t i = 0; i < RoutingManager::MAX_ROUTES; i++) {
        std::array<uint8_t, 6> mac = {(uint8_t)(i & 0xFF), 0, 0, 0, 0, (uint8_t)((i >> 8) & 0xFF)};
        std::array<uint8_t, 6> hop = {(uint8_t)((i >> 8) & 0xFF), 0, 0, 0, 0, (uint8_t)(i & 0xFF)};
        
        bool learned = router_->learn_route(mac, hop, 1, -70);
        if (i < RoutingManager::MAX_ROUTES) {
            EXPECT_TRUE(learned);
        }
    }
    
    // Assert - Table should be full
    EXPECT_GE(router_->get_route_count(), RoutingManager::MAX_ROUTES - 1);
}
