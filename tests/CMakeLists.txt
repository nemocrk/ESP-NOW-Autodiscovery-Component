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
# Real Component Source Files
# ============================================
set(REAL_COMPONENT_SOURCES
  ../components/esp_mesh/mesh.cpp
)

# ============================================
# Test Executable Configuration
# ============================================
set(TEST_SOURCES
  src/test_esp_mesh_core.cpp
  src/test_esp_mesh_networking.cpp
  src/test_esp_mesh_routing.cpp
  src/test_esp_mesh_peer_mgmt.cpp
  src/test_esp_mesh_validation.cpp
  src/test_mesh_real.cpp
)

# Add test executable
add_executable(esp_mesh_tests 
  ${TEST_SOURCES}
  ${REAL_COMPONENT_SOURCES}
)

# Link against gtest AND gmock
target_link_libraries(esp_mesh_tests PRIVATE
  gtest
  gmock
  gtest_main
)

# Include directories
target_include_directories(esp_mesh_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/../components/esp_mesh
  ${CMAKE_SOURCE_DIR}/../esphome
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# C++ standard
set_target_properties(esp_mesh_tests PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)

# Compiler flags: treat warnings as errors EXCEPT for unused variables in test code
target_compile_options(esp_mesh_tests PRIVATE
  -Wall -Wextra -Werror 
  -Wno-error=unused-variable
  -Wno-error=unused-parameter
)

# Define compilation flags
target_compile_definitions(esp_mesh_tests PRIVATE
  IS_NODE=1
)

# Register tests with gtest_discover_tests
gtest_discover_tests(esp_mesh_tests
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
