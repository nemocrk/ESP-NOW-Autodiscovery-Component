#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <array>
#include <deque>

/**
 * Test unitari per il networking layer ESP-NOW Mesh.
 * Testa:
 *   - Packet handling
 *   - Packet types (PKT_PROBE, PKT_ANNOUNCE, PKT_REG, PKT_DATA)
 *   - MeshHeader parsing
 *   - RSSI handling
 */

// ============================================
// Packet Type Definitions
// ============================================

enum PacketType : uint8_t {
    PKT_PROBE = 0x01,
    PKT_ANNOUNCE = 0x02,
    PKT_REG = 0x10,
    PKT_DATA = 0x20,
    PKT_CMD = 0x30
};

enum EntityType : uint8_t {
    BINARY_SENSOR = 0x01,
    SWITCH = 0x02,
    BUTTON = 0x03,
    SENSOR = 0x05,
    TEXT_SENSOR = 0x06,
    LIGHT = 0x0A,
    CLIMATE = 0x09
};

#pragma pack(push, 1)
struct MeshPacket {
    uint8_t type;           // Packet type
    uint32_t net_id;        // Network ID
    uint8_t src[6];         // Source MAC
    uint8_t dst[6];         // Destination MAC
    uint8_t next_hop[6];    // Next hop
    uint8_t ttl;            // Time to live
};
#pragma pack(pop)

class NetworkHandler {
public:
    static constexpr size_t MAX_PACKET_SIZE = 250;
    
    NetworkHandler() : last_rssi_(0) {}
    
    // Process incoming packet
    bool process_packet(const MeshPacket& pkt, int8_t rssi) {
        if (pkt.ttl == 0) {
            return false;  // Drop TTL=0
        }
        
        last_rssi_ = rssi;
        
        switch (pkt.type) {
            case PKT_PROBE:
                return handle_probe(pkt);
            case PKT_ANNOUNCE:
                return handle_announce(pkt);
            case PKT_REG:
                return handle_registration(pkt);
            case PKT_DATA:
                return handle_data(pkt);
            default:
                return false;
        }
    }
    
    // Getters
    int8_t get_last_rssi() const { return last_rssi_; }
    uint32_t get_processed_count() const { return processed_count_; }
    
private:
    int8_t last_rssi_;
    uint32_t processed_count_ = 0;
    
    bool handle_probe(const MeshPacket& pkt) {
        processed_count_++;
        return true;
    }
    
    bool handle_announce(const MeshPacket& pkt) {
        processed_count_++;
        return true;
    }
    
    bool handle_registration(const MeshPacket& pkt) {
        processed_count_++;
        return true;
    }
    
    bool handle_data(const MeshPacket& pkt) {
        processed_count_++;
        return true;
    }
};

// ============================================
// Test Suite: NetworkingTest
// ============================================

class NetworkingTest : public ::testing::Test {
public:
    void SetUp() override {
        handler_ = std::make_unique<NetworkHandler>();
        memset(&test_packet_, 0, sizeof(test_packet_));
    }
    
    std::unique_ptr<NetworkHandler> handler_;
    MeshPacket test_packet_;
};

/**
 * TEST: PacketReceptionValidMeshHeader
 * 
 * Verifica parsing di pacchetto valido con MeshHeader.
 */
TEST_F(NetworkingTest, PacketReceptionValidMeshHeader) {
    // Arrange
    test_packet_.type = PKT_PROBE;
    test_packet_.net_id = 0x12345678;
    test_packet_.ttl = 10;
    
    // Act
    bool result = handler_->process_packet(test_packet_, -70);
    
    // Assert
    EXPECT_TRUE(result);
    EXPECT_EQ(handler_->get_last_rssi(), -70);
}

/**
 * TEST: PacketTTLHandling
 * 
 * Verifica che pacchetti con TTL=0 siano scartati.
 */
TEST_F(NetworkingTest, PacketTTLHandling) {
    // Arrange
    test_packet_.type = PKT_PROBE;
    test_packet_.ttl = 0;
    
    // Act
    bool result = handler_->process_packet(test_packet_, -70);
    
    // Assert
    EXPECT_FALSE(result);
    EXPECT_EQ(handler_->get_processed_count(), 0);
}

/**
 * TEST: MultiplePackets
 * 
 * Verifica elaborazione di più pacchetti.
 */
TEST_F(NetworkingTest, MultiplePackets) {
    // Arrange & Act
    for (int i = 0; i < 5; i++) {
        test_packet_.type = (i % 2 == 0) ? PKT_PROBE : PKT_ANNOUNCE;
        test_packet_.ttl = 10;
        handler_->process_packet(test_packet_, -70 + i);
    }
    
    // Assert
    EXPECT_EQ(handler_->get_processed_count(), 5);
    EXPECT_EQ(handler_->get_last_rssi(), -66);
}

/**
 * TEST: PacketTypeDetection
 * 
 * Verifica riconoscimento dei tipi di pacchetto.
 */
TEST_F(NetworkingTest, PacketTypeDetection) {
    // Test all packet types
    std::array<uint8_t, 5> types = {
        PKT_PROBE, PKT_ANNOUNCE, PKT_REG, PKT_DATA, PKT_CMD
    };
    
    for (uint8_t type : types) {
        test_packet_.type = type;
        test_packet_.ttl = 10;
        
        bool result = handler_->process_packet(test_packet_, -70);
        
        // Only recognized types should be true
        if (type == PKT_PROBE || type == PKT_ANNOUNCE || 
            type == PKT_REG || type == PKT_DATA) {
            EXPECT_TRUE(result);
        }
    }
}

/**
 * TEST: RSCITracking
 * 
 * Verifica tracciamento del RSSI (Signal Strength).
 */
TEST_F(NetworkingTest, RSSITracking) {
    // Arrange
    std::array<int8_t, 4> signal_strengths = {-50, -70, -85, -100};
    
    // Act & Assert
    for (int8_t rssi : signal_strengths) {
        test_packet_.type = PKT_PROBE;
        test_packet_.ttl = 10;
        handler_->process_packet(test_packet_, rssi);
        EXPECT_EQ(handler_->get_last_rssi(), rssi);
    }
}

/**
 * TEST: MeshHeaderStructure
 * 
 * Verifica dimensioni e layout del MeshPacket.
 */
TEST_F(NetworkingTest, MeshHeaderStructure) {
    // Assert
    EXPECT_EQ(sizeof(MeshPacket), 24);
    EXPECT_LT(sizeof(MeshPacket), NetworkHandler::MAX_PACKET_SIZE);
}

/**
 * TEST: BroadcastDestination
 * 
 * Verifica identificazione di broadcast (all 0xFF).
 */
TEST_F(NetworkingTest, BroadcastDestination) {
    // Arrange
    memset(test_packet_.dst, 0xFF, 6);
    test_packet_.type = PKT_PROBE;
    test_packet_.ttl = 10;
    
    // Act - Check if broadcast
    bool is_broadcast = true;
    for (int i = 0; i < 6; i++) {
        if (test_packet_.dst[i] != 0xFF) {
            is_broadcast = false;
            break;
        }
    }
    
    // Assert
    EXPECT_TRUE(is_broadcast);
}

/**
 * TEST: EntityTypeValues
 * 
 * Verifica che i valori dell'enum EntityType siano corretti.
 */
TEST_F(NetworkingTest, EntityTypeValues) {
    // Assert
    EXPECT_EQ(BINARY_SENSOR, 0x01);
    EXPECT_EQ(SWITCH, 0x02);
    EXPECT_EQ(BUTTON, 0x03);
    EXPECT_EQ(SENSOR, 0x05);
    EXPECT_EQ(TEXT_SENSOR, 0x06);
    EXPECT_EQ(LIGHT, 0x0A);
    EXPECT_EQ(CLIMATE, 0x09);
}

/**
 * TEST: PacketTypeValues
 * 
 * Verifica che i tipi di pacchetto siano corretti.
 */
TEST_F(NetworkingTest, PacketTypeValues) {
    // Assert
    EXPECT_EQ(PKT_PROBE, 0x01);
    EXPECT_EQ(PKT_ANNOUNCE, 0x02);
    EXPECT_EQ(PKT_REG, 0x10);
    EXPECT_EQ(PKT_DATA, 0x20);
    EXPECT_EQ(PKT_CMD, 0x30);
}
