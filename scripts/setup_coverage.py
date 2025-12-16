#!/usr/bin/env python3
"""
PlatformIO Pre-Build Script for Coverage Setup
Ensures gcov coverage files are properly configured
"""

import os
Import("env")

def setup_coverage(source, target, env):
    """Setup coverage environment"""
    print("[Coverage] Setting up code coverage environment...")
    
    # Add gcov library to linker
    env.Append(
        LINKFLAGS=[
            "--coverage",
            "-lgcov"
        ]
    )
    
    # Ensure coverage directory exists
    coverage_dir = os.path.join(env['PROJECT_DIR'], '.pio', 'coverage')
    os.makedirs(coverage_dir, exist_ok=True)
    
    print(f"[Coverage] Coverage data will be stored in: {coverage_dir}")
    print("[Coverage] Run 'lcov' after tests to generate report")

# Register the callback
env.AddPreAction("buildprog", setup_coverage)
