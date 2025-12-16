#include <gtest/gtest.h>
#include "../components/esp_mesh/mesh_logic.h"
#include <cstring>
#include <vector>

using namespace esphome::esp_mesh;

/**
 * Google Test Suite for MeshLogic (Pure Logic, NO Dependencies)
 * 
 * Testing strategy:
 *   1. Configuration & Setters
 *   2. Core Algorithms (hashing, key derivation)
 *   3. Packet Validation
 *   4. Route Management
 *   5. Peer Management
 *   6. Edge Cases & Security
 */

class MeshLogicTest : public ::testing::Test {
public:
    MeshLogicTest() : mesh() {}

    void SetUp() override {
        mesh.clear_peers();
    }

    MeshLogic mesh;
};

// ============================================
// TEST SUITE 1: Configuration & Setters
// ============================================

TEST_F(MeshLogicTest, SetMeshID) {
    // Arrange
    std::string mesh_id = "SmartHome";
    uint32_t expected_hash = MeshLogic::djb2_hash(mesh_id);

    // Act
    mesh.set_mesh_id(mesh_id);

    // Assert
    EXPECT_EQ(mesh.get_net_id_hash(), expected_hash);
    EXPECT_NE(mesh.get_net_id_hash(), 0);
}

TEST_F(MeshLogicTest, SetPMKValid) {
    // Arrange
    std::string pmk = "1234567890ABCDEF";  // 16 bytes

    // Act
    mesh.set_pmk(pmk);

    // Assert
    EXPECT_EQ(mesh.get_pmk(), pmk);
    EXPECT_EQ(mesh.get_pmk().length(), 16);
}

TEST_F(MeshLogicTest, SetPMKInvalidLength) {
    // Arrange
    std::string pmk_short = "1234567890ABCDE";   // 15 bytes - INVALID
    std::string pmk_long = "1234567890ABCDEF0";  // 17 bytes - INVALID

    // Act
    mesh.set_pmk(pmk_short);
    std::string pmk_after_short = mesh.get_pmk();
    mesh.set_pmk(pmk_long);
    std::string pmk_after_long = mesh.get_pmk();

    // Assert - should NOT change on invalid input
    EXPECT_EQ(pmk_after_short.length(), 0);
    EXPECT_EQ(pmk_after_long.length(), 0);
}

TEST_F(MeshLogicTest, SetChannelValid) {
    // Act & Assert
    for (uint8_t ch = 1; ch <= 13; ch++) {
        mesh.set_channel(ch);
        EXPECT_EQ(mesh.get_current_channel(), ch)
            << "Channel " << (int)ch << " should be set";
    }
}

TEST_F(MeshLogicTest, SetChannelInvalid) {
    // Arrange
    mesh.set_channel(6);
    uint8_t original = mesh.get_current_channel();

    // Act - try invalid channels
    mesh.set_channel(0);
    mesh.set_channel(14);
    mesh.set_channel(255);

    // Assert - should not change
    EXPECT_EQ(mesh.get_current_channel(), original);
}

// ============================================
// TEST SUITE 2: Core Algorithms
// ============================================

TEST_F(MeshLogicTest, DJB2HashDeterministic) {
    // Arrange
    std::string id = "TestNetwork";

    // Act
    uint32_t hash1 = MeshLogic::djb2_hash(id);
    uint32_t hash2 = MeshLogic::djb2_hash(id);
    uint32_t hash3 = MeshLogic::djb2_hash(id);

    // Assert
    EXPECT_EQ(hash1, hash2);
    EXPECT_EQ(hash2, hash3);
}

TEST_F(MeshLogicTest, DJB2HashCollisionUnlikely) {
    // Arrange - multiple different inputs
    std::vector<std::string> ids = {"Mesh1", "Mesh2", "Test", "Home", "Office"};

    // Act
    std::vector<uint32_t> hashes;
    for (const auto &id : ids) {
        hashes.push_back(MeshLogic::djb2_hash(id));
    }

    // Assert - check for collisions (should be very unlikely)
    for (size_t i = 0; i < hashes.size(); i++) {
        for (size_t j = i + 1; j < hashes.size(); j++) {
            EXPECT_NE(hashes[i], hashes[j])
                << "Hash collision between " << ids[i] << " and " << ids[j];
        }
    }
}

TEST_F(MeshLogicTest, DeriveLMKDeterministic) {
    // Arrange
    mesh.set_pmk("1234567890ABCDEF");
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t lmk1[16], lmk2[16];

    // Act
    mesh.derive_lmk(mac, lmk1);
    mesh.derive_lmk(mac, lmk2);

    // Assert
    EXPECT_EQ(memcmp(lmk1, lmk2, 16), 0)
        << "LMK must be deterministic for same PMK and MAC";
}

TEST_F(MeshLogicTest, DeriveLMKDifferentMAC) {
    // Arrange
    mesh.set_pmk("1234567890ABCDEF");
    uint8_t mac1[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t mac2[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint8_t lmk1[16], lmk2[16];

    // Act
    mesh.derive_lmk(mac1, lmk1);
    mesh.derive_lmk(mac2, lmk2);

    // Assert
    EXPECT_NE(memcmp(lmk1, lmk2, 16), 0)
        << "Different MACs must produce different LMKs";
}

TEST_F(MeshLogicTest, DeriveLMKFormula) {
    // Arrange
    std::string pmk_str = "1234567890ABCDEF";
    mesh.set_pmk(pmk_str);
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t lmk[16];

    // Act
    mesh.derive_lmk(mac, lmk);

    // Assert - verify XOR formula
    for (int i = 0; i < 16; i++) {
        uint8_t expected = pmk_str[i] ^ mac[i % 6];
        EXPECT_EQ(lmk[i], expected) << "LMK[" << i << "] XOR mismatch";
    }
}

// ============================================
// TEST SUITE 3: Packet Validation
// ============================================

TEST_F(MeshLogicTest, ValidatePacketSize) {
    // Assert
    EXPECT_FALSE(MeshLogic::validate_packet_size(10));
    EXPECT_FALSE(MeshLogic::validate_packet_size(23));
    EXPECT_TRUE(MeshLogic::validate_packet_size(24));
    EXPECT_TRUE(MeshLogic::validate_packet_size(100));
}

TEST_F(MeshLogicTest, ValidatePacketHeader) {
    // Arrange
    mesh.set_mesh_id("TestNet");
    MeshHeader header;
    header.net_id = mesh.get_net_id_hash();
    header.ttl = 1;

    // Act & Assert
    EXPECT_TRUE(mesh.validate_packet_header(&header));

    // Wrong net_id
    header.net_id = 0xDEADBEEF;
    EXPECT_FALSE(mesh.validate_packet_header(&header));

    // Restore and test TTL = 0
    header.net_id = mesh.get_net_id_hash();
    header.ttl = 0;
    EXPECT_FALSE(mesh.validate_packet_header(&header));
}

TEST_F(MeshLogicTest, IsVirtualRoot) {
    // Arrange
    uint8_t virtual_root[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t normal_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    // Assert
    EXPECT_TRUE(MeshLogic::is_virtual_root(virtual_root));
    EXPECT_FALSE(MeshLogic::is_virtual_root(normal_mac));
    EXPECT_FALSE(MeshLogic::is_virtual_root(nullptr));
}

TEST_F(MeshLogicTest, IsBroadcast) {
    // Arrange
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t normal_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    // Assert
    EXPECT_TRUE(MeshLogic::is_broadcast(broadcast));
    EXPECT_FALSE(MeshLogic::is_broadcast(normal_mac));
    EXPECT_FALSE(MeshLogic::is_broadcast(nullptr));
}

TEST_F(MeshLogicTest, MACEqual) {
    // Arrange
    uint8_t mac1[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t mac2[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t mac3[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    // Assert
    EXPECT_TRUE(MeshLogic::mac_equal(mac1, mac2));
    EXPECT_FALSE(MeshLogic::mac_equal(mac1, mac3));
    EXPECT_FALSE(MeshLogic::mac_equal(mac1, nullptr));
}

// ============================================
// TEST SUITE 4: Route Management
// ============================================

TEST_F(MeshLogicTest, LearnRoute) {
    // Arrange
    uint8_t src[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t next_hop[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    // Act
    mesh.learn_route(src, next_hop, 1000);

    // Assert
    EXPECT_EQ(mesh.get_route_count(), 1);
    const uint8_t *found = mesh.find_route(src);
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(memcmp(found, next_hop, 6), 0);
}

TEST_F(MeshLogicTest, UpdateRoute) {
    // Arrange
    uint8_t src[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t hop1[6] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    uint8_t hop2[6] = {0x22, 0x22, 0x22, 0x22, 0x22, 0x22};

    // Act
    mesh.learn_route(src, hop1, 1000);
    mesh.learn_route(src, hop2, 1100);

    // Assert - should have 1 route, updated with hop2
    EXPECT_EQ(mesh.get_route_count(), 1);
    const uint8_t *found = mesh.find_route(src);
    EXPECT_EQ(memcmp(found, hop2, 6), 0);
}

TEST_F(MeshLogicTest, GCOldRoutes) {
    // Arrange
    uint8_t src1[6] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    uint8_t src2[6] = {0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB};
    uint8_t hop[6] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

    mesh.learn_route(src1, hop, 1000);  // Old
    mesh.learn_route(src2, hop, 400000);  // Recent

    // Act
    mesh.gc_old_routes(700000);  // Current time: 700 seconds

    // Assert - src1 should be garbage collected
    EXPECT_EQ(mesh.get_route_count(), 1);
    EXPECT_EQ(mesh.find_route(src1), nullptr);
    EXPECT_NE(mesh.find_route(src2), nullptr);
}

// ============================================
// TEST SUITE 5: Peer Management
// ============================================

TEST_F(MeshLogicTest, AddPeer) {
    // Arrange
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t lmk[16] = {0};

    // Act
    mesh.add_peer(mac, lmk);

    // Assert
    EXPECT_TRUE(mesh.peer_exists(mac));
    EXPECT_EQ(mesh.get_peer_count(), 1);
}

TEST_F(MeshLogicTest, PeerLRUEviction) {
    // Arrange - Add MAX_PEERS + 1
    uint8_t mac_base[6] = {0xAA, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t lmk[16] = {0};

    // Act - Add MAX_PEERS
    for (int i = 0; i < MAX_PEERS; i++) {
        mac_base[1] = i;
        mesh.add_peer(mac_base, lmk);
    }

    EXPECT_EQ(mesh.get_peer_count(), MAX_PEERS);

    // Add one more - should evict first
    mac_base[1] = 0xFF;
    mesh.add_peer(mac_base, lmk);

    // Assert
    EXPECT_EQ(mesh.get_peer_count(), MAX_PEERS);
}

TEST_F(MeshLogicTest, PeerUpdateLRU) {
    // Arrange
    uint8_t mac1[6] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    uint8_t mac2[6] = {0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB};
    uint8_t lmk[16] = {0};

    // Act
    mesh.add_peer(mac1, lmk);
    mesh.add_peer(mac2, lmk);
    mesh.add_peer(mac1, lmk);  // Update mac1 - should be moved to end

    // Assert - Still 2 peers
    EXPECT_EQ(mesh.get_peer_count(), 2);
    EXPECT_TRUE(mesh.peer_exists(mac1));
    EXPECT_TRUE(mesh.peer_exists(mac2));
}

TEST_F(MeshLogicTest, ClearPeers) {
    // Arrange
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t lmk[16] = {0};
    mesh.add_peer(mac, lmk);

    // Act
    mesh.clear_peers();

    // Assert
    EXPECT_EQ(mesh.get_peer_count(), 0);
    EXPECT_FALSE(mesh.peer_exists(mac));
}

// ============================================
// TEST SUITE 6: Structure Size Validation
// ============================================

TEST_F(MeshLogicTest, MeshHeaderStructSize) {
    EXPECT_EQ(sizeof(MeshHeader), 24);
}

TEST_F(MeshLogicTest, RegPayloadStructSize) {
    EXPECT_EQ(sizeof(RegPayload), 53);
}

TEST_F(MeshLogicTest, EnumValues) {
    EXPECT_EQ(PKT_PROBE, 0x01);
    EXPECT_EQ(PKT_ANNOUNCE, 0x02);
    EXPECT_EQ(PKT_REG, 0x10);
    EXPECT_EQ(PKT_DATA, 0x20);
    EXPECT_EQ(PKT_CMD, 0x30);

    EXPECT_EQ(ENTITY_TYPE_BINARY_SENSOR, 0x01);
    EXPECT_EQ(ENTITY_TYPE_SENSOR, 0x05);
    EXPECT_EQ(ENTITY_TYPE_SWITCH, 0x02);
    EXPECT_EQ(ENTITY_TYPE_LIGHT, 0x0A);
}
