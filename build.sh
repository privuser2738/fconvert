#!/bin/bash
# fconvert Linux/macOS Build Script

BUILD_TYPE="Release"
BUILD_DIR="build"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        debug|Debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        release|Release)
            BUILD_TYPE="Release"
            shift
            ;;
        clean)
            echo "Cleaning build directory..."
            rm -rf "$BUILD_DIR"
            echo "Clean complete."
            exit 0
            ;;
        rebuild)
            echo "Rebuilding..."
            rm -rf "$BUILD_DIR"
            shift
            ;;
        *)
            shift
            ;;
    esac
done

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found. Please install CMake."
    exit 1
fi

# Check for compiler
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo "ERROR: No C++ compiler found. Please install g++ or clang++."
    exit 1
fi

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Configure with CMake
echo ""
echo "Configuring CMake ($BUILD_TYPE)..."
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed."
    exit 1
fi

# Build
echo ""
echo "Building fconvert..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed."
    exit 1
fi

echo ""
echo "Build successful!"
echo "Binary: $BUILD_DIR/fconvert"
exit 0
