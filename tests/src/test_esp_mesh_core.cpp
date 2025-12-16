#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>
#include <string>
#include <array>

/**
 * Test unitari per il componente core ESP-NOW Mesh.
 * Testa:
 *   - Inizializzazione NODE vs ROOT
 *   - Setter di configurazione (mesh_id, pmk, channel)
 *   - Computazione hash DJB2
 *   - Priority di setup
 */

// ============================================
// Minimal EspMesh Class for Testing
// ============================================

class EspMesh {
public:
    enum Mode { NODE, ROOT };
    
    uint8_t hop_count_;
    bool scanning_;
    std::string pmk_;
    uint32_t net_id_hash_;
    std::array<uint8_t, 6> my_mac_;
    uint8_t current_scan_ch_;
    Mode mode_;
    
    EspMesh(Mode mode) : mode_(mode) {
        if (mode == NODE) {
            hop_count_ = 0xFF;      // Not connected
            scanning_ = true;
            current_scan_ch_ = 1;
        } else {
            hop_count_ = 0;         // ROOT is coordinator
            scanning_ = false;
            current_scan_ch_ = 1;
        }
        pmk_ = "";
        net_id_hash_ = 0;
        my_mac_.fill(0);
    }
    
    virtual ~EspMesh() = default;
    
    // Setup function
    bool setup() {
        if (pmk_.length() != 16) {
            return false;  // PMK must be 16 chars
        }
        
        if (mode_ == ROOT) {
            // Simulate getting MAC from ESP32
            my_mac_[0] = 0xAA;
            my_mac_[1] = 0xBB;
            my_mac_[2] = 0xCC;
            my_mac_[3] = 0xDD;
            my_mac_[4] = 0xEE;
            my_mac_[5] = 0xFF;
        }
        
        return true;
    }
    
    // Setter functions
    void set_pmk(const std::string& pmk) {
        if (pmk.length() == 16) {
            pmk_ = pmk;
        }
    }
    
    void set_mesh_id(const std::string& mesh_id) {
        net_id_hash_ = djb2_hash(mesh_id);
    }
    
    void set_channel(uint8_t channel) {
        if (channel >= 1 && channel <= 13) {
            current_scan_ch_ = channel;
        }
    }
    
    // Hash computation
    static uint32_t djb2_hash(const std::string& s) {
        uint32_t h = 5381;
        for (char c : s) {
            h = ((h << 5) + h) + static_cast<unsigned char>(c);
        }
        return h;
    }
    
    // Configuration dump
    std::string dump_config() const {
        std::string config = "EspMesh Config: ";
        config += (mode_ == NODE) ? "NODE" : "ROOT";
        config += " | hop_count=" + std::to_string(hop_count_);
        config += " | scanning=" + std::string(scanning_ ? "true" : "false");
        return config;
    }
    
    // Get setup priority
    float get_setup_priority() const {
        return (mode_ == NODE) ? 4.0f : -50.0f;
    }
};

// ============================================
// Test Suite: EspMeshSetup
// ============================================

class EspMeshSetupTest : public ::testing::Test {
public:
    void SetUp() override {
        node_mesh_ = std::make_unique<EspMesh>(EspMesh::NODE);
        root_mesh_ = std::make_unique<EspMesh>(EspMesh::ROOT);
    }

    void TearDown() override {
        node_mesh_.reset();
        root_mesh_.reset();
    }
    
    std::unique_ptr<EspMesh> node_mesh_;
    std::unique_ptr<EspMesh> root_mesh_;
};

/**
 * TEST: NodeSetupInitialization
 * 
 * Verifica che un NODE si inizializzi correttamente.
 */
TEST_F(EspMeshSetupTest, NodeSetupInitialization) {
    // Arrange
    ASSERT_NE(node_mesh_, nullptr);
    
    // Act
    node_mesh_->set_pmk("1234567890ABCDEF");
    node_mesh_->set_mesh_id("SmartHome_Mesh");
    bool setup_ok = node_mesh_->setup();
    
    // Assert
    EXPECT_TRUE(setup_ok);
    EXPECT_EQ(node_mesh_->hop_count_, 0xFF);
    EXPECT_TRUE(node_mesh_->scanning_);
    EXPECT_EQ(node_mesh_->pmk_.length(), 16);
    EXPECT_NE(node_mesh_->net_id_hash_, 0);
}

/**
 * TEST: RootSetupInitialization
 * 
 * Verifica che un ROOT si inizializzi correttamente.
 */
TEST_F(EspMeshSetupTest, RootSetupInitialization) {
    // Arrange
    ASSERT_NE(root_mesh_, nullptr);
    
    // Act
    root_mesh_->set_pmk("1234567890ABCDEF");
    root_mesh_->set_mesh_id("SmartHome_Mesh");
    bool setup_ok = root_mesh_->setup();
    
    // Assert
    EXPECT_TRUE(setup_ok);
    EXPECT_EQ(root_mesh_->hop_count_, 0);
    EXPECT_FALSE(root_mesh_->scanning_);
    EXPECT_EQ(root_mesh_->my_mac_[0], 0xAA);
    EXPECT_EQ(root_mesh_->my_mac_[5], 0xFF);
}

/**
 * TEST: PMKSetterValidation
 * 
 * Verifica che set_pmk() memorizzi correttamente la PMK.
 */
TEST_F(EspMeshSetupTest, PMKSetterValidation) {
    // Arrange
    std::string valid_pmk = "1234567890ABCDEF";
    std::string invalid_pmk = "Short";
    
    // Act & Assert - Valid PMK
    node_mesh_->set_pmk(valid_pmk);
    EXPECT_EQ(node_mesh_->pmk_, valid_pmk);
    EXPECT_EQ(node_mesh_->pmk_.length(), 16);
    
    // Act & Assert - Invalid PMK (wrong length)
    node_mesh_->set_pmk(invalid_pmk);
    EXPECT_EQ(node_mesh_->pmk_, valid_pmk);  // Unchanged
}

/**
 * TEST: PMKSetupRequired
 * 
 * Verifica che setup() fallisca senza PMK valida.
 */
TEST_F(EspMeshSetupTest, PMKSetupRequired) {
    // Arrange
    node_mesh_->set_mesh_id("SmartHome_Mesh");
    
    // Act - Setup without PMK
    bool setup_ok = node_mesh_->setup();
    
    // Assert
    EXPECT_FALSE(setup_ok);
}

/**
 * TEST: MeshIdHashComputation
 * 
 * Verifica che djb2_hash() computi correttamente l'hash della mesh_id.
 */
TEST_F(EspMeshSetupTest, MeshIdHashComputation) {
    // Arrange
    std::string mesh_id_1 = "SmartHome_Mesh";
    std::string mesh_id_2 = "Another_Mesh";
    
    // Act
    uint32_t hash1 = EspMesh::djb2_hash(mesh_id_1);
    uint32_t hash1_again = EspMesh::djb2_hash(mesh_id_1);
    uint32_t hash2 = EspMesh::djb2_hash(mesh_id_2);
    
    // Assert
    EXPECT_EQ(hash1, hash1_again);  // Deterministic
    EXPECT_NE(hash1, 0);             // Non-zero
    EXPECT_NE(hash1, hash2);         // Different mesh_ids → different hashes
}

/**
 * TEST: MeshIdSetter
 * 
 * Verifica che set_mesh_id() computi e memorizzi il hash.
 */
TEST_F(EspMeshSetupTest, MeshIdSetter) {
    // Arrange
    std::string mesh_id = "MyNetwork";
    uint32_t expected_hash = EspMesh::djb2_hash(mesh_id);
    
    // Act
    node_mesh_->set_mesh_id(mesh_id);
    
    // Assert
    EXPECT_EQ(node_mesh_->net_id_hash_, expected_hash);
    EXPECT_NE(node_mesh_->net_id_hash_, 0);
}

/**
 * TEST: ChannelSetterNode
 * 
 * Verifica che set_channel() impostI il canale per i NODE.
 */
TEST_F(EspMeshSetupTest, ChannelSetterNode) {
    // Arrange
    uint8_t test_channel = 6;
    
    // Act
    node_mesh_->set_channel(test_channel);
    
    // Assert
    EXPECT_EQ(node_mesh_->current_scan_ch_, 6);
}

/**
 * TEST: ChannelSetterValidRange
 * 
 * Verifica che set_channel() accetti solo canali 1-13.
 */
TEST_F(EspMeshSetupTest, ChannelSetterValidRange) {
    // Arrange - Valid channels
    for (uint8_t ch = 1; ch <= 13; ch++) {
        // Act
        node_mesh_->set_channel(ch);
        
        // Assert
        EXPECT_EQ(node_mesh_->current_scan_ch_, ch);
    }
    
    // Test invalid channel
    uint8_t original = node_mesh_->current_scan_ch_;
    node_mesh_->set_channel(14);
    EXPECT_EQ(node_mesh_->current_scan_ch_, original);  // Unchanged
}

// ============================================
// Test Suite: MeshHeaderStructure
// ============================================

class MeshHeaderStructureTest : public ::testing::Test {};

#pragma pack(push, 1)
struct MeshHeader {
    uint8_t type;                  // 1 byte
    uint32_t net_id;               // 4 bytes
    uint8_t src[6];                // 6 bytes
    uint8_t dst[6];                // 6 bytes
    uint8_t next_hop[6];           // 6 bytes
    uint8_t ttl;                   // 1 byte
};
#pragma pack(pop)

/**
 * TEST: MeshHeaderSize
 * 
 * Verifica che MeshHeader abbia dimensione corretta (24 bytes).
 */
TEST_F(MeshHeaderStructureTest, MeshHeaderSize) {
    // Assert
    EXPECT_EQ(sizeof(MeshHeader), 24);
}

/**
 * TEST: MeshHeaderPacking
 * 
 * Verifica che i campi siano packed correttamente.
 */
TEST_F(MeshHeaderStructureTest, MeshHeaderPacking) {
    // Arrange
    MeshHeader header;
    header.type = 0x01;
    header.net_id = 0x12345678;
    header.ttl = 10;
    memset(header.src, 0xAA, 6);
    memset(header.dst, 0xBB, 6);
    memset(header.next_hop, 0xCC, 6);
    
    // Assert
    EXPECT_EQ(header.type, 0x01);
    EXPECT_EQ(header.net_id, 0x12345678);
    EXPECT_EQ(header.ttl, 10);
    EXPECT_EQ(header.src[0], 0xAA);
    EXPECT_EQ(header.dst[0], 0xBB);
}

// ============================================
// Test Suite: DumpConfig
// ============================================

class DumpConfigTest : public ::testing::Test {
public:
    void SetUp() override {
        node_mesh_ = std::make_unique<EspMesh>(EspMesh::NODE);
        root_mesh_ = std::make_unique<EspMesh>(EspMesh::ROOT);
    }
    
    std::unique_ptr<EspMesh> node_mesh_;
    std::unique_ptr<EspMesh> root_mesh_;
};

/**
 * TEST: DumpConfigNode
 * 
 * Verifica che dump_config() contenga informazioni corrette per NODE.
 */
TEST_F(DumpConfigTest, DumpConfigNode) {
    // Act
    std::string config = node_mesh_->dump_config();
    
    // Assert
    EXPECT_NE(config.find("NODE"), std::string::npos);
    EXPECT_NE(config.find("hop_count=255"), std::string::npos);
    EXPECT_NE(config.find("scanning=true"), std::string::npos);
}

/**
 * TEST: DumpConfigRoot
 * 
 * Verifica che dump_config() contenga informazioni corrette per ROOT.
 */
TEST_F(DumpConfigTest, DumpConfigRoot) {
    // Act
    std::string config = root_mesh_->dump_config();
    
    // Assert
    EXPECT_NE(config.find("ROOT"), std::string::npos);
    EXPECT_NE(config.find("hop_count=0"), std::string::npos);
    EXPECT_NE(config.find("scanning=false"), std::string::npos);
}

// ============================================
// Test Suite: SetupPriority
// ============================================

class SetupPriorityTest : public ::testing::Test {
public:
    void SetUp() override {
        node_mesh_ = std::make_unique<EspMesh>(EspMesh::NODE);
        root_mesh_ = std::make_unique<EspMesh>(EspMesh::ROOT);
    }
    
    std::unique_ptr<EspMesh> node_mesh_;
    std::unique_ptr<EspMesh> root_mesh_;
};

/**
 * TEST: NodeSetupPriority
 * 
 * Verifica che NODE abbia setup_priority = WIFI (setup presto).
 */
TEST_F(SetupPriorityTest, NodeSetupPriority) {
    // Act
    float priority = node_mesh_->get_setup_priority();
    
    // Assert
    EXPECT_EQ(priority, 4.0f);  // WIFI priority
}

/**
 * TEST: RootSetupPriority
 * 
 * Verifica che ROOT abbia setup_priority = AFTER_WIFI (dopo WiFi).
 */
TEST_F(SetupPriorityTest, RootSetupPriority) {
    // Act
    float priority = root_mesh_->get_setup_priority();
    
    // Assert
    EXPECT_EQ(priority, -50.0f);  // AFTER_WIFI priority
    EXPECT_LT(priority, node_mesh_->get_setup_priority());
}

/**
 * TEST: PriorityOrdering
 * 
 * Verifica che le priorità siano ordinate correttamente.
 */
TEST_F(SetupPriorityTest, PriorityOrdering) {
    // NODE deve avere priorità maggiore (viene prima)
    float node_priority = node_mesh_->get_setup_priority();
    float root_priority = root_mesh_->get_setup_priority();
    
    EXPECT_GT(node_priority, root_priority);
}
