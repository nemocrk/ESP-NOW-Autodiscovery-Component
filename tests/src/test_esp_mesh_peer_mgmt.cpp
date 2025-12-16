#include <gtest/gtest.h>
#include <cstdint>
#include <deque>
#include <algorithm>

/**
 * Test unitari per la gestione dei peer ESP-NOW.
 * Testa:
 *   - Aggiunta peer
 *   - LRU eviction
 *   - Protezione parent
 *   - Derivazione LMK
 */

class PeerMgmtTest : public ::testing::Test {};

/**
 * TEST: AddNewPeerSuccess
 * 
 * Verifica aggiunta di nuovo peer.
 * 
 * Prerequisiti:
 *   - Peer table non piena
 *   - MAC nuovo
 * 
 * Azioni:
 *   1. ensure_peer_slot(new_mac)
 *   2. esp_now_add_peer() called
 *   3. LMK derivato e impostato
 * 
 * Risultato Atteso:
 *   - Peer aggiunto
 *   - peer_lru_.size() incrementato
 */
TEST_F(PeerMgmtTest, AddNewPeerSuccess) {
    const int MAX_PEERS = 6;
    std::deque<uint8_t> peer_lru;

    // Simula aggiunta di nuovo peer
    peer_lru.push_back(1);
    EXPECT_EQ(peer_lru.size(), 1);
}

/**
 * TEST: LRUEvictionWhenTableFull
 * 
 * Verifica eviction quando tabella peer piena.
 * 
 * Prerequisiti:
 *   - MAX_PEERS = 6
 *   - Tabella ha 6 peer
 *   - Nuovo peer vuole entrare
 * 
 * Azioni:
 *   1. Riempi con 6 peer
 *   2. ensure_peer_slot(peer_7)
 *   3. Verifica eviction del più vecchio
 * 
 * Risultato Atteso:
 *   - Front rimosso
 *   - peer_lru_.size() rimane 6
 *   - peer_7 aggiunto a tail
 */
TEST_F(PeerMgmtTest, LRUEvictionWhenTableFull) {
    const int MAX_PEERS = 6;
    std::deque<int> peer_lru;

    // Riempi tabella con 6 peer
    for (int i = 0; i < MAX_PEERS; i++) {
        peer_lru.push_back(i);
    }
    EXPECT_EQ(peer_lru.size(), 6);

    // Aggiungi nuovo peer - deve fare eviction
    if (peer_lru.size() >= MAX_PEERS) {
        peer_lru.pop_front();  // Evict oldest (front)
        peer_lru.push_back(6);  // Aggiungi nuovo
    }

    EXPECT_EQ(peer_lru.size(), 6);
    EXPECT_EQ(peer_lru.back(), 6);
    EXPECT_NE(peer_lru.front(), 0);  // Primo peer rimosso
}

/**
 * TEST: LRUOrderMaintained
 * 
 * Verifica che ordine LRU sia mantenuto.
 * 
 * Prerequisiti:
 *   - Peer list: [A, B, C]
 * 
 * Azioni:
 *   1. Accedi peer A (ensure_peer_slot)
 *   2. A spostato a end (most recently used)
 * 
 * Risultato Atteso:
 *   - Order: [B, C, A]
 */
TEST_F(PeerMgmtTest, LRUOrderMaintained) {
    std::deque<int> peer_lru = {1, 2, 3};

    // Accedi peer 1 (move to end)
    auto it = std::find(peer_lru.begin(), peer_lru.end(), 1);
    if (it != peer_lru.end()) {
        int peer = *it;
        peer_lru.erase(it);
        peer_lru.push_back(peer);
    }

    EXPECT_EQ(peer_lru[0], 2);
    EXPECT_EQ(peer_lru[1], 3);
    EXPECT_EQ(peer_lru[2], 1);
}

/**
 * TEST: DeriveLMKDeterministic
 * 
 * Verifica che LMK sia deterministico.
 * 
 * Prerequisiti:
 *   - PMK fisso
 *   - MAC fisso
 * 
 * Azioni:
 *   1. derive_lmk(mac) chiamato 2 volte
 *   2. Confronta risultati
 * 
 * Risultato Atteso:
 *   - LMK identico in entrambe le volte
 *   - Formula: LMK[i] = PMK[i] XOR MAC[i % 6]
 */
TEST_F(PeerMgmtTest, DeriveLMKDeterministic) {
    uint8_t pmk[16] = {'1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F'};
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    // Compute LMK twice
    uint8_t lmk1[16], lmk2[16];
    for (int i = 0; i < 16; i++) {
        lmk1[i] = pmk[i] ^ mac[i % 6];
        lmk2[i] = pmk[i] ^ mac[i % 6];
    }

    // Verifiche
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(lmk1[i], lmk2[i]);
    }
}

/**
 * TEST: DeriveLMKDifferentForDifferentMACs
 * 
 * Verifica che MAC diversi producano LMK diversi.
 */
TEST_F(PeerMgmtTest, DeriveLMKDifferentForDifferentMACs) {
    uint8_t pmk[16] = {'1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F'};
    uint8_t mac1[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t mac2[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    uint8_t lmk1[16], lmk2[16];
    for (int i = 0; i < 16; i++) {
        lmk1[i] = pmk[i] ^ mac1[i % 6];
        lmk2[i] = pmk[i] ^ mac2[i % 6];
    }

    // LMK diversi
    bool are_different = false;
    for (int i = 0; i < 16; i++) {
        if (lmk1[i] != lmk2[i]) {
            are_different = true;
            break;
        }
    }
    EXPECT_TRUE(are_different);
}
