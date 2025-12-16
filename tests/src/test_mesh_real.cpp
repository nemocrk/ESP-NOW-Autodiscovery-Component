#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>
#include <array>

// ============================================
// CRITICAL: Include mock headers BEFORE real code
// ============================================
#include "../mocks/esphome_mock.h"
#include "../mocks/esp_now_mock.h"
#include "../mocks/esp_wifi_mock.h"

// ============================================
// Define compilation flags for NODE mode
// ============================================
#define IS_NODE 1

// ============================================
// Include real component headers
// ============================================
#include "../../components/esp_mesh/mesh.h"

using namespace esphome::esp_mesh;

// ============================================
// Test Suite: Real EspMesh Component
// ============================================

class RealEspMeshTest : public ::testing::Test {
public:
    void SetUp() override {
        // Reset all mocks before each test
        EspNowMock::instance().reset();
        EspWiFiMock::instance().reset();
        test_reset_timer();
    }

    void TearDown() override {
        // Cleanup after each test
    }
};

/**
 * TEST: DJB2 Hash Computation (Real Function)
 * 
 * Verifica che la funzione reale EspMesh::djb2_hash()
 * computi correttamente l'hash.
 */
TEST_F(RealEspMeshTest, DJB2HashComputation) {
    // Arrange
    EspMesh mesh;
    std::string mesh_id_1 = "SmartHome_Mesh";
    std::string mesh_id_2 = "Another_Mesh";

    // Act - Test determinism
    uint32_t hash1 = mesh.djb2_hash(mesh_id_1);
    uint32_t hash1_again = mesh.djb2_hash(mesh_id_1);
    uint32_t hash2 = mesh.djb2_hash(mesh_id_2);

    // Assert
    EXPECT_EQ(hash1, hash1_again) << "DJB2 hash must be deterministic";
    EXPECT_NE(hash1, 0) << "Hash should not be zero";
    EXPECT_NE(hash1, hash2) << "Different inputs should produce different hashes";
}

/**
 * TEST: PMK Setter (Real Function)
 * 
 * Verifica che set_pmk() memorizzi correttamente il PMK.
 */
TEST_F(RealEspMeshTest, PMKSetter) {
    // Arrange
    EspMesh mesh;
    std::string valid_pmk = "1234567890ABCDEF";

    // Act
    mesh.set_pmk(valid_pmk);

    // Assert
    EXPECT_EQ(mesh.pmk_, valid_pmk) << "PMK should be stored correctly";
    EXPECT_EQ(mesh.pmk_.length(), 16) << "PMK must be exactly 16 characters";
}

/**
 * TEST: Mesh ID Setter (Real Function)
 * 
 * Verifica che set_mesh_id() computi e memorizzi il hash.
 */
TEST_F(RealEspMeshTest, MeshIDSetter) {
    // Arrange
    EspMesh mesh;
    std::string mesh_id = "TestNetwork";
    uint32_t expected_hash = mesh.djb2_hash(mesh_id);

    // Act
    mesh.set_mesh_id(mesh_id);

    // Assert
    EXPECT_EQ(mesh.net_id_hash_, expected_hash) << "Mesh ID hash should be computed";
    EXPECT_NE(mesh.net_id_hash_, 0) << "Hash should not be zero";
}

/**
 * TEST: Channel Setter (Real Function)
 * 
 * Verifica che set_channel() accetti solo canali validi 1-13.
 */
TEST_F(RealEspMeshTest, ChannelSetterValidRange) {
    // Arrange
    EspMesh mesh;
    mesh.current_scan_ch_ = 1;  // Initial channel

    // Act & Assert - Valid channels
    for (uint8_t ch = 1; ch <= 13; ch++) {
        mesh.set_channel(ch);
        EXPECT_EQ(mesh.current_scan_ch_, ch)
            << "Valid channel " << (int)ch << " should be set";
    }
}

/**
 * TEST: Get Setup Priority NODE (Real Function)
 * 
 * Verifica che get_setup_priority() ritorni WIFI per NODE.
 */
TEST_F(RealEspMeshTest, GetSetupPriorityNODE) {
    // Arrange
    EspMesh mesh;

    // Act
    float priority = mesh.get_setup_priority();

    // Assert
    EXPECT_EQ(priority, setup_priority::WIFI)
        << "NODE should have WIFI setup priority";
}

/**
 * TEST: Setup NODE (Real Function - Partial)
 * 
 * Verifica che setup() NODE initializzi correttamente
 * con i mock delle ESP APIs.
 */
TEST_F(RealEspMeshTest, SetupNODE) {
    // Arrange
    EspMesh mesh;
    mesh.set_pmk("1234567890ABCDEF");
    mesh.set_mesh_id("TestMesh");

    // Pre-check
    EXPECT_EQ(EspNowMock::instance().initialized, false)
        << "ESP-NOW should not be initialized yet";

    // Act
    mesh.setup();

    // Assert - ESP-NOW initialization
    EXPECT_TRUE(EspNowMock::instance().initialized)
        << "ESP-NOW should be initialized after setup()";

    // Assert - PMK was set correctly
    uint8_t expected_pmk[16];
    std::string pmk_str = "1234567890ABCDEF";
    memcpy(expected_pmk, pmk_str.c_str(), 16);
    EXPECT_EQ(memcmp(EspNowMock::instance().pmk, expected_pmk, 16), 0)
        << "PMK should be set in ESP-NOW";

    // Assert - Callback registered
    EXPECT_NE(EspNowMock::instance().recv_callback, nullptr)
        << "Receive callback should be registered";

    // Assert - Not failed
    EXPECT_FALSE(mesh.is_failed()) << "Mesh should not be marked failed";
}

/**
 * TEST: Dump Config NODE (Real Function)
 * 
 * Verifica che dump_config() stampi informazioni corrette.
 */
TEST_F(RealEspMeshTest, DumpConfigNODE) {
    // Arrange
    EspMesh mesh;
    mesh.set_pmk("1234567890ABCDEF");
    mesh.set_mesh_id("TestMesh");
    mesh.setup();

    // Act & Assert
    // dump_config() just logs - verify it doesn't crash
    EXPECT_NO_THROW(mesh.dump_config());
}

/**
 * TEST: Hash Collision Test
 * 
 * Verifica che hash di stringhe diverse siano diversi
 * (non ci siano collisioni intenzionali).
 */
TEST_F(RealEspMeshTest, HashNoCollision) {
    // Arrange
    EspMesh mesh;
    std::vector<std::string> test_ids = {
        "Mesh1", "Mesh2", "TestNet", "Home", "Office", "Garden", "Garage"
    };

    // Act
    std::vector<uint32_t> hashes;
    for (const auto& id : test_ids) {
        hashes.push_back(mesh.djb2_hash(id));
    }

    // Assert - Check for collisions
    for (size_t i = 0; i < hashes.size(); i++) {
        for (size_t j = i + 1; j < hashes.size(); j++) {
            EXPECT_NE(hashes[i], hashes[j])
                << "Hash collision between " << test_ids[i] << " and "
                << test_ids[j];
        }
    }
}

/**
 * TEST: MeshHeader Structure Size
 * 
 * Verifica che MeshHeader abbia il size corretto (24 bytes).
 */
TEST_F(RealEspMeshTest, MeshHeaderSize) {
    // Assert - Structure size
    EXPECT_EQ(sizeof(MeshHeader), 24) << "MeshHeader must be 24 bytes (packed)";

    // Assert - Field offsets
    MeshHeader h;
    EXPECT_EQ(sizeof(h.type), 1);
    EXPECT_EQ(sizeof(h.net_id), 4);
    EXPECT_EQ(sizeof(h.src), 6);
    EXPECT_EQ(sizeof(h.dst), 6);
    EXPECT_EQ(sizeof(h.next_hop), 6);
    EXPECT_EQ(sizeof(h.ttl), 1);
}

/**
 * TEST: Entity Type Enum Values
 * 
 * Verifica che i valori dell'enum EntityType siano corretti.
 */
TEST_F(RealEspMeshTest, EntityTypeValues) {
    // Assert
    EXPECT_EQ(ENTITY_TYPE_BINARY_SENSOR, 0x01);
    EXPECT_EQ(ENTITY_TYPE_SWITCH, 0x02);
    EXPECT_EQ(ENTITY_TYPE_BUTTON, 0x03);
    EXPECT_EQ(ENTITY_TYPE_SENSOR, 0x05);
    EXPECT_EQ(ENTITY_TYPE_TEXT_SENSOR, 0x06);
    EXPECT_EQ(ENTITY_TYPE_LIGHT, 0x0A);
    EXPECT_EQ(ENTITY_TYPE_CLIMATE, 0x09);
}

/**
 * TEST: Packet Type Enum Values
 * 
 * Verifica che i valori dell'enum PktType siano corretti.
 */
TEST_F(RealEspMeshTest, PacketTypeValues) {
    // Assert
    EXPECT_EQ(PKT_PROBE, 0x01);
    EXPECT_EQ(PKT_ANNOUNCE, 0x02);
    EXPECT_EQ(PKT_REG, 0x10);
    EXPECT_EQ(PKT_DATA, 0x20);
    EXPECT_EQ(PKT_CMD, 0x30);
}

/**
 * TEST: RegPayload Structure Size
 * 
 * Verifica dimensione della RegPayload.
 */
TEST_F(RealEspMeshTest, RegPayloadSize) {
    // Assert - Structure exists and has expected fields
    RegPayload p;
    EXPECT_EQ(sizeof(p.entity_hash), 4);
    EXPECT_EQ(sizeof(p.type_id), 1);
    EXPECT_EQ(sizeof(p.name), 24);
    EXPECT_EQ(sizeof(p.unit), 8);
    EXPECT_EQ(sizeof(p.dev_class), 16);
}
