#pragma once

/**
 * ESP-NOW Mesh Core Logic (NO External Dependencies)
 * 
 * Questa classe contiene SOLO la logica critica del mesh:
 *   - DJB2 hashing
 *   - PMK derivation (LMK)
 *   - Packet validation
 *   - Route tracking
 *   - Peer management logic
 * 
 * Niente dipendenze ESPHome, niente SDK, pura logica C++.
 */

#include <cstdint>
#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <algorithm>

namespace esphome {
namespace esp_mesh {

// ============================================
// Constants & Type Definitions
// ============================================

const uint8_t MAX_PEERS = 20;
const uint32_t ROUTE_TIMEOUT_MS = 300000;  // 5 minuti

enum PktType : uint8_t {
    PKT_PROBE = 0x01,
    PKT_ANNOUNCE = 0x02,
    PKT_REG = 0x10,
    PKT_DATA = 0x20,
    PKT_CMD = 0x30,
};

enum EntityType : uint8_t {
    ENTITY_TYPE_BINARY_SENSOR = 0x01,
    ENTITY_TYPE_SWITCH = 0x02,
    ENTITY_TYPE_BUTTON = 0x03,
    ENTITY_TYPE_SENSOR = 0x05,
    ENTITY_TYPE_TEXT_SENSOR = 0x06,
    ENTITY_TYPE_LIGHT = 0x0A,
    ENTITY_TYPE_CLIMATE = 0x09,
    ENTITY_TYPE_FAN = 0x0F,
    ENTITY_TYPE_COVER = 0x08,
    ENTITY_TYPE_NUMBER = 0x0C,
};

// ============================================
// Packed Structures
// ============================================

struct __attribute__((packed)) MeshHeader {
    uint8_t type;
    uint32_t net_id;
    uint8_t src[6];
    uint8_t dst[6];
    uint8_t next_hop[6];
    uint8_t ttl;
};

static_assert(sizeof(MeshHeader) == 24, "MeshHeader must be 24 bytes (packed)");

struct __attribute__((packed)) RegPayload {
    uint32_t entity_hash;
    uint8_t type_id;
    char name[24];
    char unit[8];
    char dev_class[16];
};

static_assert(sizeof(RegPayload) == 53, "RegPayload size must be 53 bytes");

struct RouteInfo {
    uint8_t next_hop[6];
    uint32_t last_seen_ms;
};

struct PeerInfo {
    uint8_t mac[6];
    uint8_t lmk[16];
};

// ============================================
// Core Mesh Logic Class (Pure Logic)
// ============================================

class MeshLogic {
public:
    MeshLogic() : net_id_hash_(0), hop_count_(0xFF) {}

    virtual ~MeshLogic() {}

    // ========================================
    // Configuration & Setters
    // ========================================

    /**
     * Set mesh ID and compute its hash.
     */
    void set_mesh_id(const std::string &id) {
        net_id_hash_ = djb2_hash(id);
    }

    /**
     * Set Pre-Shared Key (must be 16 bytes).
     */
    void set_pmk(const std::string &pmk) {
        if (pmk.length() != 16) {
            return;  // Invalid PMK
        }
        pmk_ = pmk;
    }

    /**
     * Set WiFi scan channel (1-13).
     */
    void set_channel(uint8_t channel) {
        if (channel >= 1 && channel <= 13) {
            current_scan_ch_ = channel;
        }
    }

    // ========================================
    // Core Algorithms
    // ========================================

    /**
     * DJB2 Hash Function (deterministic).
     * Used for mesh ID -> network ID conversion.
     */
    static uint32_t djb2_hash(const std::string &s) {
        uint32_t h = 5381;
        for (char c : s) {
            h = ((h << 5) + h) + static_cast<uint8_t>(c);
        }
        return h;  // Already uint32 due to overflow semantics
    }

    /**
     * Derive Local Mesh Key (LMK) from PMK and peer MAC.
     * Formula: LMK[i] = PMK[i] XOR MAC[i % 6]
     */
    void derive_lmk(const uint8_t *mac, uint8_t *lmk) const {
        if (!mac || !lmk) return;
        for (int i = 0; i < 16; i++) {
            lmk[i] = pmk_[i] ^ mac[i % 6];
        }
    }

    // ========================================
    // Packet Validation
    // ========================================

    /**
     * Validate if MeshHeader has correct magic and net_id.
     */
    bool validate_packet_header(const MeshHeader *header) const {
        if (!header) return false;
        if (header->net_id != net_id_hash_) return false;
        if (header->ttl == 0) return false;  // Dead packet
        return true;
    }

    /**
     * Validate if packet has minimum size.
     */
    static bool validate_packet_size(int len) {
        return len >= static_cast<int>(sizeof(MeshHeader));
    }

    /**
     * Check if MAC is all zeros (virtual root).
     */
    static bool is_virtual_root(const uint8_t *mac) {
        if (!mac) return false;
        for (int i = 0; i < 6; i++) {
            if (mac[i] != 0) return false;
        }
        return true;
    }

    /**
     * Check if MAC is broadcast.
     */
    static bool is_broadcast(const uint8_t *mac) {
        if (!mac) return false;
        return mac[0] == 0xFF;
    }

    /**
     * Check if two MACs are equal.
     */
    static bool mac_equal(const uint8_t *mac1, const uint8_t *mac2) {
        if (!mac1 || !mac2) return false;
        return memcmp(mac1, mac2, 6) == 0;
    }

    // ========================================
    // Route Management
    // ========================================

    /**
     * Learn reverse path from packet source.
     */
    void learn_route(const uint8_t *src_mac, const uint8_t *next_hop, uint32_t now_ms) {
        if (!src_mac || !next_hop) return;

        std::string key = mac_to_string(src_mac);
        RouteInfo &route = routes_[key];
        memcpy(route.next_hop, next_hop, 6);
        route.last_seen_ms = now_ms;
    }

    /**
     * Garbage collect old routes (timeout > ROUTE_TIMEOUT_MS).
     */
    void gc_old_routes(uint32_t now_ms) {
        auto it = routes_.begin();
        while (it != routes_.end()) {
            if (now_ms - it->second.last_seen_ms > ROUTE_TIMEOUT_MS) {
                it = routes_.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * Find next hop to reach destination MAC.
     * Returns next_hop MAC, or nullptr if unknown.
     */
    const uint8_t *find_route(const uint8_t *dst_mac) const {
        if (!dst_mac) return nullptr;
        std::string key = mac_to_string(dst_mac);
        auto it = routes_.find(key);
        if (it != routes_.end()) {
            return it->second.next_hop;
        }
        return nullptr;
    }

    // ========================================
    // Peer Management
    // ========================================

    /**
     * Check if peer already exists in LRU cache.
     */
    bool peer_exists(const uint8_t *mac) const {
        if (!mac) return false;
        std::string key = mac_to_string(mac);
        return peers_.find(key) != peers_.end();
    }

    /**
     * Add or update peer in LRU cache.
     */
    void add_peer(const uint8_t *mac, const uint8_t *lmk) {
        if (!mac) return;

        std::string key = mac_to_string(mac);

        // Remove if already exists (update LRU position)
        auto it = peer_lru_.find(key);
        if (it != peer_lru_.end()) {
            peer_lru_.erase(it);
        }

        // If cache full, evict LRU
        if (peer_lru_.size() >= MAX_PEERS) {
            std::string lru_key = *peer_lru_.begin();
            peers_.erase(lru_key);
            peer_lru_.erase(lru_key);
        }

        // Add peer
        PeerInfo peer;
        memcpy(peer.mac, mac, 6);
        if (lmk) {
            memcpy(peer.lmk, lmk, 16);
        } else {
            memset(peer.lmk, 0, 16);
        }
        peers_[key] = peer;

        // Update LRU (iteration order)
        peer_lru_[key] = true;
    }

    /**
     * Get peer count.
     */
    size_t get_peer_count() const {
        return peers_.size();
    }

    /**
     * Clear all peers.
     */
    void clear_peers() {
        peers_.clear();
        peer_lru_.clear();
    }

    // ========================================
    // Getters
    // ========================================

    uint32_t get_net_id_hash() const { return net_id_hash_; }
    const std::string &get_pmk() const { return pmk_; }
    uint8_t get_current_channel() const { return current_scan_ch_; }
    uint8_t get_hop_count() const { return hop_count_; }
    size_t get_route_count() const { return routes_.size(); }

    // ========================================
    // Helpers
    // ========================================

    static std::string mac_to_string(const uint8_t *mac) {
        if (!mac) return "00:00:00:00:00:00";
        char buf[18];
        snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
                 mac[1], mac[2], mac[3], mac[4], mac[5]);
        return std::string(buf);
    }

protected:
    // Configuration
    uint32_t net_id_hash_;
    std::string pmk_;
    uint8_t current_scan_ch_ = 1;
    uint8_t hop_count_;

    // Routing table
    std::map<std::string, RouteInfo> routes_;

    // Peer cache (LRU)
    std::map<std::string, PeerInfo> peers_;
    std::map<std::string, bool> peer_lru_;  // Insertion order
};

}  // namespace esp_mesh
}  // namespace esphome
