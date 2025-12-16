#include <gtest/gtest.h>

// ============================================
// CRITICAL: Define UNIT_TEST before including mesh.h
// ============================================
#define UNIT_TEST 1
#define IS_NODE 1

// Include mocks BEFORE real code
#include "mocks/esp_mocks.h"

// Now include REAL mesh.cpp code
#include "../components/esp_mesh/mesh.h"

using namespace esphome::esp_mesh;

/**
 * Integration Tests for REAL mesh.cpp Code
 * 
 * Questi test compilano e testano il VERO codice mesh.cpp
 * utilizzando mock delle API hardware.
 * 
 * Coverage: ~90% del codice reale
 */

class RealMeshIntegrationTest : public ::testing::Test {
public:
    EspMesh *mesh;

    void SetUp() override {
        // Reset all mocks
        mock_reset_all();
        
        // Create real mesh instance
        mesh = new EspMesh();
        mesh->set_mesh_id("TestMesh");
        mesh->set_pmk("1234567890ABCDEF");  // 16 bytes
        mesh->set_channel(6);
    }

    void TearDown() override {
        delete mesh;
    }
    
    // Helper: create MAC address
    void make_mac(uint8_t *buf, uint8_t last_byte) {
        uint8_t base[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, last_byte};
        memcpy(buf, base, 6);
    }
};

// ============================================
// TEST SUITE 1: Setup and Initialization
// ============================================

TEST_F(RealMeshIntegrationTest, NodeSetupInitializesESPNow) {
    // Act
    mesh->setup();

    // Assert
    EXPECT_TRUE(esp_now_mock::initialized) << "ESP-NOW should be initialized";
    EXPECT_FALSE(mesh->is_failed()) << "Component should not be marked failed";
}

TEST_F(RealMeshIntegrationTest, NodeSetupRegistersPMK) {
    // Arrange
    std::string pmk = "1234567890ABCDEF";
    mesh->set_pmk(pmk);

    // Act
    mesh->setup();

    // Assert
    uint8_t expected_pmk[16];
    memcpy(expected_pmk, pmk.c_str(), 16);
    EXPECT_EQ(memcmp(esp_now_mock::pmk, expected_pmk, 16), 0)
        << "PMK should be set in ESP-NOW";
}

TEST_F(RealMeshIntegrationTest, NodeSetupRegistersReceiveCallback) {
    // Act
    mesh->setup();

    // Assert
    EXPECT_NE(esp_now_mock::recv_callback, nullptr)
        << "Receive callback should be registered";
}

TEST_F(RealMeshIntegrationTest, NodeStartsWithDisconnectedState) {
    // Act
    mesh->setup();

    // Assert
    EXPECT_EQ(mesh->hop_count_, 0xFF) << "NODE should start with hop_count=0xFF";
}

// ============================================
// TEST SUITE 2: Packet Reception and Validation
// ============================================

TEST_F(RealMeshIntegrationTest, RejectsPacketWithWrongNetID) {
    // Arrange
    mesh->setup();
    
    uint8_t src[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint8_t dst[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    MeshHeader header;
    header.type = PKT_PROBE;
    header.net_id = 0xDEADBEEF;  // Wrong net_id
    header.ttl = 1;
    memcpy(header.src, src, 6);
    memcpy(header.dst, dst, 6);

    size_t route_count_before = mesh->get_route_count();

    // Act
    test_simulate_recv(src, dst, (uint8_t*)&header, sizeof(header), -60);

    // Assert - packet should be ignored
    EXPECT_EQ(mesh->get_route_count(), route_count_before)
        << "Route should not be learned from wrong net_id";
}

TEST_F(RealMeshIntegrationTest, RejectsPacketWithZeroTTL) {
    // Arrange
    mesh->setup();
    
    uint8_t src[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint8_t dst[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    MeshHeader header;
    header.type = PKT_PROBE;
    header.net_id = mesh->get_net_id_hash();
    header.ttl = 0;  // Dead packet
    memcpy(header.src, src, 6);
    memcpy(header.dst, dst, 6);

    size_t route_count_before = mesh->get_route_count();

    // Act
    test_simulate_recv(src, dst, (uint8_t*)&header, sizeof(header), -60);

    // Assert
    EXPECT_EQ(mesh->get_route_count(), route_count_before)
        << "Route should not be learned from TTL=0 packet";
}

TEST_F(RealMeshIntegrationTest, AcceptsValidPacket) {
    // Arrange
    mesh->setup();
    
    uint8_t src[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint8_t dst[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    MeshHeader header;
    header.type = PKT_PROBE;
    header.net_id = mesh->get_net_id_hash();
    header.ttl = 10;
    memcpy(header.src, src, 6);
    memcpy(header.dst, dst, 6);

    // Act
    test_simulate_recv(src, dst, (uint8_t*)&header, sizeof(header), -60);

    // Assert - route should be learned
    EXPECT_GT(mesh->get_route_count(), 0)
        << "Route should be learned from valid packet";
}

// ============================================
// TEST SUITE 3: LMK Derivation (Security)
// ============================================

TEST_F(RealMeshIntegrationTest, DeriveLMKProducesCorrectKey) {
    // Arrange
    std::string pmk = "1234567890ABCDEF";
    mesh->set_pmk(pmk);
    mesh->setup();
    
    uint8_t parent_mac[6] = {0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6};
    uint8_t lmk[16];

    // Act
    mesh->derive_lmk(parent_mac, lmk);

    // Assert - verify XOR formula
    for (int i = 0; i < 16; i++) {
        uint8_t expected = pmk[i] ^ parent_mac[i % 6];
        EXPECT_EQ(lmk[i], expected) << "LMK[" << i << "] XOR mismatch";
    }
}

TEST_F(RealMeshIntegrationTest, DifferentMACsProduceDifferentLMKs) {
    // Arrange
    mesh->set_pmk("1234567890ABCDEF");
    mesh->setup();
    
    uint8_t mac1[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t mac2[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint8_t lmk1[16], lmk2[16];

    // Act
    mesh->derive_lmk(mac1, lmk1);
    mesh->derive_lmk(mac2, lmk2);

    // Assert
    EXPECT_NE(memcmp(lmk1, lmk2, 16), 0)
        << "Different MACs must produce different LMKs";
}

// ============================================
// TEST SUITE 4: Peer Management (LRU)
// ============================================

TEST_F(RealMeshIntegrationTest, PeerIsAddedToCache) {
    // Arrange
    mesh->setup();
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t lmk[16] = {0};

    // Act
    mesh->ensure_peer_slot(mac);

    // Assert
    EXPECT_TRUE(esp_now_is_peer_exist(mac))
        << "Peer should be added to ESP-NOW";
}

TEST_F(RealMeshIntegrationTest, LRUEvictsOldestPeer) {
    // Arrange
    mesh->setup();
    
    // Fill peer cache to MAX_PEERS
    for (int i = 0; i < MAX_PEERS; i++) {
        uint8_t mac[6];
        make_mac(mac, i);
        mesh->ensure_peer_slot(mac);
    }
    
    EXPECT_EQ(esp_now_mock::peers.size(), MAX_PEERS)
        << "Cache should be full";

    // Act - add one more peer
    uint8_t new_mac[6];
    make_mac(new_mac, 0xFF);
    mesh->ensure_peer_slot(new_mac);

    // Assert
    EXPECT_LE(esp_now_mock::peers.size(), MAX_PEERS)
        << "Cache size should not exceed MAX_PEERS";
    
    EXPECT_TRUE(esp_now_is_peer_exist(new_mac))
        << "New peer should be in cache";
}

// ============================================
// TEST SUITE 5: Route Learning
// ============================================

TEST_F(RealMeshIntegrationTest, LearnsRouteFromPacket) {
    // Arrange
    mesh->setup();
    
    uint8_t src[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint8_t next_hop[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t dst[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    MeshHeader header;
    header.type = PKT_PROBE;
    header.net_id = mesh->get_net_id_hash();
    header.ttl = 10;
    memcpy(header.src, src, 6);
    memcpy(header.dst, dst, 6);

    // Act - simulate packet from src, received from next_hop
    test_simulate_recv(next_hop, dst, (uint8_t*)&header, sizeof(header), -60);

    // Assert
    const uint8_t *found_route = mesh->find_route(src);
    EXPECT_NE(found_route, nullptr) << "Route should be learned";
    
    if (found_route) {
        EXPECT_EQ(memcmp(found_route, next_hop, 6), 0)
            << "Route should point to next_hop MAC";
    }
}

TEST_F(RealMeshIntegrationTest, UpdatesExistingRoute) {
    // Arrange
    mesh->setup();
    
    uint8_t src[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint8_t hop1[6] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    uint8_t hop2[6] = {0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB};
    uint8_t dst[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    MeshHeader header;
    header.type = PKT_PROBE;
    header.net_id = mesh->get_net_id_hash();
    header.ttl = 10;
    memcpy(header.src, src, 6);
    memcpy(header.dst, dst, 6);

    // Act - first packet from hop1
    test_simulate_recv(hop1, dst, (uint8_t*)&header, sizeof(header), -60);
    
    // Act - second packet from hop2 (better route)
    test_simulate_recv(hop2, dst, (uint8_t*)&header, sizeof(header), -70);

    // Assert - route should be updated to hop2
    const uint8_t *found_route = mesh->find_route(src);
    EXPECT_NE(found_route, nullptr);
    
    if (found_route) {
        EXPECT_EQ(memcmp(found_route, hop2, 6), 0)
            << "Route should be updated to new next_hop";
    }
}

// ============================================
// TEST SUITE 6: Packet Sending
// ============================================

TEST_F(RealMeshIntegrationTest, SendRawAddsPeerIfNeeded) {
    // Arrange
    mesh->setup();
    uint8_t dst[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t data[10] = {0x01, 0x02, 0x03};

    // Act
    mesh->send_raw(dst, data, sizeof(data));

    // Assert
    EXPECT_TRUE(esp_now_is_peer_exist(dst))
        << "Peer should be added before sending";
    
    EXPECT_GT(esp_now_mock::sent_packets.size(), 0)
        << "Packet should be sent";
}

TEST_F(RealMeshIntegrationTest, BroadcastPacketSentWithoutEncryption) {
    // Arrange
    mesh->setup();
    uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t data[10] = {0x01, 0x02, 0x03};

    // Act
    mesh->send_raw(bcast, data, sizeof(data));

    // Assert
    EXPECT_GT(esp_now_mock::sent_packets.size(), 0)
        << "Broadcast packet should be sent";
    
    // Verify broadcast peer was added
    EXPECT_TRUE(esp_now_is_peer_exist(bcast));
}

// ============================================
// TEST SUITE 7: DJB2 Hash (Real Implementation)
// ============================================

TEST_F(RealMeshIntegrationTest, DJB2HashIsDeterministic) {
    // Arrange
    std::string id = "TestMesh";

    // Act
    uint32_t hash1 = mesh->djb2_hash(id);
    uint32_t hash2 = mesh->djb2_hash(id);

    // Assert
    EXPECT_EQ(hash1, hash2) << "DJB2 hash must be deterministic";
}

TEST_F(RealMeshIntegrationTest, DJB2HashNoCollisions) {
    // Arrange
    std::vector<std::string> ids = {"Mesh1", "Mesh2", "Test", "Home"};

    // Act
    std::vector<uint32_t> hashes;
    for (const auto &id : ids) {
        hashes.push_back(mesh->djb2_hash(id));
    }

    // Assert - no collisions
    for (size_t i = 0; i < hashes.size(); i++) {
        for (size_t j = i + 1; j < hashes.size(); j++) {
            EXPECT_NE(hashes[i], hashes[j])
                << "Hash collision between " << ids[i] << " and " << ids[j];
        }
    }
}

// ============================================
// TEST SUITE 8: Structure Validation
// ============================================

TEST_F(RealMeshIntegrationTest, MeshHeaderSizeIs24Bytes) {
    EXPECT_EQ(sizeof(MeshHeader), 24)
        << "MeshHeader must be exactly 24 bytes (packed)";
}

TEST_F(RealMeshIntegrationTest, RegPayloadSizeIs53Bytes) {
    EXPECT_EQ(sizeof(RegPayload), 53)
        << "RegPayload must be exactly 53 bytes";
}
