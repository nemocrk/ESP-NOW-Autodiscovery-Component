cmake_minimum_required(VERSION 3.16)
project(esp_mesh_tests CXX)

# ============================================
# Google Test Setup
# ============================================
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# CRITICAL: Include GoogleTest module for gtest_discover_tests
include(GoogleTest)

enable_testing()

# ============================================
# Test Executable Configuration
# ============================================
set(TEST_SOURCES
  src/test_esp_mesh_core.cpp
  src/test_esp_mesh_networking.cpp
  src/test_esp_mesh_routing.cpp
  src/test_esp_mesh_peer_mgmt.cpp
  src/test_esp_mesh_validation.cpp
)

# Add test executable
add_executable(esp_mesh_tests ${TEST_SOURCES})

# Link against gtest
target_link_libraries(esp_mesh_tests PRIVATE
  gtest
  gtest_main
)

# Include directories
target_include_directories(esp_mesh_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/../esphome/components/esp_mesh
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

# C++ standard
set_target_properties(esp_mesh_tests PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)

# Register tests with gtest_discover_tests
gtest_discover_tests(esp_mesh_tests)
