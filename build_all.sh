#!/bin/bash
echo "========================================"
echo "Building Unmatched Project (Linux)"
echo "========================================"

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring CMake..."
cmake ..
if [ $? -ne 0 ]; then
    echo "[ERROR] CMake configuration failed"
    exit 1
fi

# Build the project
echo "Building project..."
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "[ERROR] Build failed"
    exit 1
fi

echo "========================================"
echo "Build completed successfully!"
echo "========================================"
echo "To run the graphical version: ./build/unmatched_graphical"
echo "To run the TUI version: ./build/unmatched_tui"