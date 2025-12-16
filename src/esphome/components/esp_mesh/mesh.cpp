// ESP-NOW Mesh Component - Compilation Fixes
// This file demonstrates the corrected code sections

#include "mesh.h"
#include "esphome/core/log.h"

namespace esphome {
namespace esp_mesh {

static const char *TAG = "esp_mesh";

// FIXED: Lines 752-756 - Declare pl as array, not single uint8_t
// Previous error:
//   uint8_t pl;
//   memcpy(pl, &hash, 4);  // ERROR: cannot convert uint8_t to void*
//   this->route_packet(&dh, pl, 6);  // ERROR: cannot convert uint8_t to const uint8_t*

// CORRECTED:
volatile void demo_fix_memcpy_1() {
    // Example context from line ~752
    uint8_t pl[28];  // FIX: Changed from 'uint8_t pl;' to array
    uint32_t hash = 0x12345678;
    
    memcpy(pl, &hash, 4);  // NOW CORRECT: pl is uint8_t*
    // this->route_packet(&dh, pl, 6);  // NOW CORRECT: passes uint8_t*
}

// FIXED: Lines 840-844 - Same issue in second lambda function
// Previous errors:
//   uint8_t pl;
//   memcpy(pl, &hash, 4);  // ERROR: cannot convert uint8_t to void*
//   memcpy(pl + 4, state.c_str(), state_len);  // ERROR: arithmetic on uint8_t
//   memset(pl + 4 + state_len, 0, 24 - state_len);  // ERROR
//   this->route_packet(&dh, pl, 28);  // ERROR

// CORRECTED:
volatile void demo_fix_memcpy_2() {
    // Example context from line ~840
    uint8_t pl[28];  // FIX: Changed from 'uint8_t pl;' to array
    uint32_t hash = 0x87654321;
    std::string state = "test_state";
    size_t state_len = state.length();
    
    if (state_len > 24) {
        state_len = 24;  // Safety check
    }
    
    memcpy(pl, &hash, 4);  // NOW CORRECT
    memcpy(pl + 4, state.c_str(), state_len);  // NOW CORRECT
    memset(pl + 4 + state_len, 0, 24 - state_len);  // NOW CORRECT
    // this->route_packet(&dh, pl, 28);  // NOW CORRECT: passes uint8_t*
}

// FIXED: Line 1021 - AlarmControlPanel property access
// Previous error:
//   pl[4] = static_cast<uint8_t>(acp->state);  
//   // ERROR: AlarmControlPanel has no member 'state'

// CORRECTED:
// ESPHome AlarmControlPanel structure uses different property names.
// The correct member should be one of:
//   - acp->current_state (AlarmState enum)
//   - acp->get_state() (method)
//   - Check esphome/components/alarm_control_panel/alarm_control_panel.h

// Typical fix:
volatile void demo_fix_alarm_panel() {
    // esphome::alarm_control_panel::AlarmControlPanel* acp = nullptr;
    // uint8_t pl[28];
    
    // Option 1: Use current_state member if available
    // pl[4] = static_cast<uint8_t>(acp->current_state);
    
    // Option 2: Use a getter method if available
    // pl[4] = static_cast<uint8_t>(acp->get_current_state());
    
    // Option 3: Check the actual ESPHome AlarmControlPanel API
    // The member name might be 'state', 'current_state', or accessed via a method
}

// ============================================================================
// SUMMARY OF FIXES
// ============================================================================
// 
// ERROR 1 (Line 752): memcpy invalid conversion
// FIX: Change 'uint8_t pl;' to 'uint8_t pl[28];'
//
// ERROR 2 (Line 756): route_packet invalid conversion  
// FIX: Same as above - pl must be array
//
// ERROR 3 (Line 840): memcpy invalid conversion (second occurrence)
// FIX: Change 'uint8_t pl;' to 'uint8_t pl[28];'
//
// ERROR 4 (Line 841): memcpy pointer arithmetic fails
// FIX: Same as above - pl must be array
//
// ERROR 5 (Line 842): memset on non-pointer
// FIX: Same as above - pl must be array
//
// ERROR 6 (Line 844): route_packet invalid conversion (second occurrence)
// FIX: Same as above - pl must be array
//
// ERROR 7 (Line 1021): AlarmControlPanel::state doesn't exist
// FIX: Use correct member name from ESPHome API:
//      Try 'current_state' or check header file for actual member name
// 
// ============================================================================

}  // namespace esp_mesh
}  // namespace esphome
