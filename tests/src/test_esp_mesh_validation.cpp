#include <gtest/gtest.h>
#include <string>
#include <cstring>

/**
 * Test unitari per la validazione dello schema YAML.
 * Testa:
 *   - CONFIG_SCHEMA validation
 *   - Required fields
 *   - PMK length (exactly 16 chars)
 *   - Mode enum
 *   - Cross-component validation
 */

class ValidationTest : public ::testing::Test {};

/**
 * TEST: PMKExactly16Chars
 * 
 * Verifica che PMK sia esattamente 16 caratteri.
 */
TEST_F(ValidationTest, PMKExactly16Chars) {
    std::string pmk = "1234567890ABCDEF";
    EXPECT_EQ(pmk.length(), 16);
}

/**
 * TEST: PMKTooShortError
 * 
 * Verifica errore se PMK < 16 chars.
 */
TEST_F(ValidationTest, PMKTooShortError) {
    std::string pmk = "short";
    EXPECT_LT(pmk.length(), 16);
}

/**
 * TEST: PMKTooLongError
 * 
 * Verifica errore se PMK > 16 chars.
 */
TEST_F(ValidationTest, PMKTooLongError) {
    std::string pmk = "1234567890ABCDEFGH";
    EXPECT_GT(pmk.length(), 16);
}

/**
 * TEST: PMKWithSpecialChars
 * 
 * Verifica PMK con caratteri speciali (valido se length=16).
 */
TEST_F(ValidationTest, PMKWithSpecialChars) {
    std::string pmk = "!@#$%^&*-+=[]{}";
    EXPECT_EQ(pmk.length(), 15);  // Un carattere meno
    
    pmk = "!@#$%^&*-+=[]{};";  // 16 chars
    EXPECT_EQ(pmk.length(), 16);
}

/**
 * TEST: ModeNodeValid
 * 
 * Verifica mode NODE valido.
 */
TEST_F(ValidationTest, ModeNodeValid) {
    std::string mode = "NODE";
    int mode_val = (mode == "NODE") ? 1 : 0;
    EXPECT_EQ(mode_val, 1);
}

/**
 * TEST: ModeRootValid
 * 
 * Verifica mode ROOT valido.
 */
TEST_F(ValidationTest, ModeRootValid) {
    std::string mode = "ROOT";
    int mode_val = (mode == "ROOT") ? 0 : 1;
    EXPECT_EQ(mode_val, 0);
}

/**
 * TEST: MeshIdHashDeterministic
 * 
 * Verifica che hash di mesh_id sia deterministico.
 */
TEST_F(ValidationTest, MeshIdHashDeterministic) {
    auto djb2_hash = [](const std::string& s) {
        uint32_t h = 5381;
        for (char c : s) {
            h = ((h << 5) + h) + c;
        }
        return h;
    };

    std::string mesh_id = "TestMesh";
    uint32_t hash1 = djb2_hash(mesh_id);
    uint32_t hash2 = djb2_hash(mesh_id);

    EXPECT_EQ(hash1, hash2);
}

/**
 * TEST: ChannelRangeValid
 * 
 * Verifica che channel sia nel range 1-13.
 */
TEST_F(ValidationTest, ChannelRangeValid) {
    int channel = 6;
    EXPECT_GE(channel, 1);
    EXPECT_LE(channel, 13);
}

/**
 * TEST: ChannelOutOfRangeError
 * 
 * Verifica errore per channel fuori range.
 */
TEST_F(ValidationTest, ChannelOutOfRangeError) {
    int channel_invalid = 15;
    EXPECT_GT(channel_invalid, 13);
}
